#include "Scene.h"

#include "Types/Object.h"

#include "Core/Camera.h"
#include "Core/Frustum.h"
#include "Core/World.h"
#include "Core/engine.h"

#include "Threading/ThreadPool.h"

#include "Components/LightManager.h"
#include "Components/RenderableManager.h"
#include "Components/TransformManager.h"

#include "utility/AlignedAlloc.h"

#include "VulkanAPI/VkDriver.h"

namespace OmegaEngine
{

Scene::Scene(World& world, Engine& engine)
    : world(world)
    , engine(engine)
{
}

Scene::~Scene()
{
}

void Scene::prepare()
{
	// prepare the camera buffer - note: the id matches the naming of the shader ubo
	// the data will be updated on a per frame basis in the update
	auto& driver = engine.getVkDriver();
	driver.addUbo("CameraUbo", sizeof(Camera::Ubo), Buffer::Usage::Dynamic);
}

void Scene::getVisibleRenderables(Frustum& frustum, std::vector<Scene::VisibleCandidate>& renderables)
{
	size_t workSize = renderables.size();

	auto visibilityCheck = [&frustum, &renderables](size_t curr_idx, size_t chunkSize) {
		assert(curr_idx + chunkSize < renderables.size());
		for (size_t idx = curr_idx; idx < curr_idx + chunkSize; ++idx)
		{
			Renderable* rend = renderables[idx].renderable;
			AABox& box = rend->instance.getAABBox();
			if (frustum.checkBoxPlaneIntersect(box))
			{
				rend->visibility |= Renderable::Visible::Renderable;
			}
		}
	};

	ThreadTaskSplitter splitWork{ 0, workSize, visibilityCheck };
	splitWork.run();
}

void Scene::update()
{
	auto& transManager = engine.getTransManager();
	auto& rendManager = engine.getRendManager();
	auto& lightManager = engine.getLightManager();

	auto& objects = world.getObjManager().getObjectsList();

	// reserve more space than we need
	std::vector<VisibleCandidate> candObjects(objects.size());

	// create a temp list of all renderable and light objects that are active
	// we create a temp container as we will be doing the visibility checks async
	for (Object& obj : objects)
	{
		if (!obj.isActive())
		{
			continue;
		}

		ObjHandle rHandle = rendManager.getObjIndex(obj);
		ObjHandle tHandle = transManager.getObjIndex(obj);

		// it should be impossible for a object to exsist that doesn't have both a transform and renderable component, but better make sure!
		if (rHandle && tHandle)
		{
			VisibleCandidate candidate;
			candidate.renderable = &rendManager.getMesh(rHandle);
			candidate.transform = &transManager.getTransform(tHandle);
			candObjects.emplace_back(candidate);
		}
		else
		{
			ObjHandle lHandle = lightManager.getObjIndex(obj);
			if (lHandle)
			{
				// TODO
			}
		}
	}

	// prepare the camera frustum
	Camera& camera = cameras[currCamera];
	Frustum frustum;
	frustum.projection(camera.getViewMatrix() * camera.getProjMatrix());

	// ============ visibility checks and culling ===================
	// first renderables - split work tasks and run async - Sets the visibility bit if passes intersection test
	// This will then be used to generate the render queue
	getVisibleRenderables(frustum, candObjects);

	// shadow culling tests

	// and prepare the visible lighting list

	// ============ render queue generation =========================
	std::vector<RenderableQueueInfo> queueRend;

	for (const VisibleCandidate& cand : candObjects)
	{
		Renderable* rend = cand.renderable;
		// only add visible renderables to the queue
		if (!rend->visibility & Renderable::Visible::Renderable)
		{
			continue;
		}

		RenderableQueueInfo queueInfo;
		// we use the renderable data as it is, rather than waste time copying everything into another struct.
		// This method does mean that it is imperative that the data isnt destroyed until the beginning of the next
		// frame ad that the data isn't written too - we aren't using guards though this might be required.
		queueInfo.renderableData = (void*)&rend;
		queueInfo.renderableHandle = this;
		queueInfo.renderFunction = GBufferFillPass::drawCallback;
		queueInfo.sortingKey =
		    RenderQueue::createSortKey(RenderQueue::Layer::Default, rend->materialId, rend->variantBits.getUint64());
		queueRend.emplace_back(queueInfo);
	}
	renderQueue.pushRenderables(queueRend, RenderQueue::Partition::Colour);

	// ================== update ubos =================================
	updateCamera();

	// transforms
	struct TransformUbo
	{
		OEMaths::mat4f modelMatrix;
	};

	struct SkinnedUbo
	{
		OEMaths::mat4f modelMatrix;
		OEMaths::mat4f jointMatrices[MAX_BONE_COUNT];
		float jointCount;
	};

	TransformUbo* currTransformPtr = nullptr;
	SkinnedUbo* currSkinnedPtr = nullptr;

	const size_t dataSize = candObjects.size();

	const size_t staticDynAlign = (sizeof(TransformUbo) + 256 - 1) & ~(256 - 1);
	const size_t skinDynAlign = (sizeof(SkinnedUbo) + 256 - 1) & ~(256 - 1);

	Util::AlignedAlloc alignAlloc{ dynAlign * dataSize, dynAlign };
	Util::AlignedAlloc alignAlloc{ dynAlign * dataSize, dynAlign };
	assert(!alignAlloc.empty());

	for (auto& cand : candObjects)
	{
		if (!rend->visibility & Renderable::Visible::Renderable)
		{
			continue;
		}

		TransformInfo* transInfo = cand.transform;
		if (!transInfo->jointCount)
		{
			currTransformPtr = (TransformUbo*)((uint64_t)transformUbo + (transUboAlign * transUboCount));
		}
		else
		{
			currSkinnedPtr = (SkinnedUbo*)((uint64_t)skinUbo + (skinUboAlign * skinUboCount));
		}
	}
}


void Scene::updateCamera()
{
	Camera& camera = cameras[currCamera];

	camera.updateViewMatrix();

	// update everything in the buffer
	Camera::Ubo ubo;
	ubo.mvp = camera.getMvpMatrix();
	ubo.cameraPosition = camera.getPos();
	ubo.projection = camera.getProjMatrix();
	ubo.model = camera.getModelMatrix();    // this is just identity for now
	ubo.view = camera.getViewMatrix();
	ubo.zNear = camera.getZNear();
	ubo.zFar = camera.getZFar();

	auto& driver = engine.getVkDriver();
	driver.updateUniform("CameraUbo", &ubo, sizeof(Camera::Ubo));
}

Camera* Scene::getCurrentCamera()
{
	return &cameras[currCamera];
}

}    // namespace OmegaEngine
