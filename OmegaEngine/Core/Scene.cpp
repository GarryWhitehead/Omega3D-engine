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

Scene::Scene(World& world, Engine& engine, VulkanAPI::VkDriver& driver)
    : world(world)
    , engine(engine)
    , driver(driver)
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

void Scene::getVisibleLights(Frustum& frustum, std::vector<LightBase*>& lights)
{
	size_t workSize = lights.size();

	auto visibilityCheck = [&frustum, &lights](size_t curr_idx, size_t chunkSize) {
		assert(curr_idx + chunkSize < lights.size());
		for (size_t idx = curr_idx; idx < curr_idx + chunkSize; ++idx)
		{
			LightBase* light = lights[idx];
			light->isVisible = false;

			if (light->type != LightType::Directional)
			{
				float radius;
				OEMaths::vec3f pos;

				if (light->type == LightType::Point)
				{
					PointLight* pLight = reinterpret_cast<PointLight*>(light);
					radius = pLight->radius;
					pos = pLight->position;
				}
				else if (light->type == LightType::Spot)
				{
					SpotLight* sLight = reinterpret_cast<SpotLight*>(light);
					radius = sLight->radius;
					pos = sLight->position;
				}
				else
				{
					LOGGER_WARN("Unrecognisied light type detected. This shouldn't happen!");
					continue;
				}

				// check whether this light is with the frustum boundaries
				if (frustum.checkSphereIntersect(pos, radius))
				{
					light->isVisible = true;
				}
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
	std::vector<VisibleCandidate> candRenderableObjs(objects.size());
	std::vector<LightBase*> candLightObjs(objects.size());

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
			candRenderableObjs.emplace_back(candidate);
		}
		else
		{
			ObjHandle lHandle = lightManager.getObjIndex(obj);
			if (lHandle)
			{
				candLightObjs.emplace_back(lightManager.getLight(lHandle));
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
	getVisibleRenderables(frustum, candRenderableObjs);

	// shadow culling tests
	// ** TODO **

	// and prepare the visible lighting list
	getVisibleLights(frustum, candLightObjs);

	// ============ render queue generation =========================
	std::vector<RenderableQueueInfo> queueRend;

	// key a count of the number of static and skinned models for later
	size_t staticModelCount = 0;
	size_t skinnedModelCount = 0;

	for (const VisibleCandidate& cand : candRenderableObjs)
	{
		Renderable* rend = cand.renderable;
		// only add visible renderables to the queue
		if (!rend->visibility & Renderable::Visible::Renderable)
		{
			continue;
		}

		if (rend->instance.hasSkin())
		{
			++skinnedModelCount;
		}
		else
		{
			++staticModelCount;
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
	// camera buffer is updated every frame as we expect this to change a lot
	updateCameraBuffer();

	// we also update the transforms every frame though could have a dirty flag
	updateTransformBuffer(staticModelCount, skinnedModelCount);
}

void Scene::updateCameraBuffer()
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

void Scene::updateTransformBuffer(std::vector<Scene::VisibleCandidate>& candObjects, const size_t staticModelCount,
                                  const size_t skinnedModelCount)
{
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

	// Dynamic buffers are aligned to >256 bytes as designated by the Vulkan spec
	const size_t staticDynAlign = (sizeof(TransformUbo) + 256 - 1) & ~(256 - 1);
	const size_t skinDynAlign = (sizeof(SkinnedUbo) + 256 - 1) & ~(256 - 1);

	Util::AlignedAlloc staticAlignAlloc{ staticDynAlign * staticModelCount, staticDynAlign };
	Util::AlignedAlloc skinAlignAlloc{ skinDynAlign * skinnedModelCount, skinDynAlign };
	assert(!staticAlignAlloc.empty());
	assert(!skinAlignAlloc.empty());

	size_t staticCount = 0;
	size_t skinnedCount = 0;

	for (auto& cand : candObjects)
	{
		Renderable* rend = cand.renderable;
		if (!rend->visibility & Renderable::Visible::Renderable)
		{
			continue;
		}

		TransformInfo* transInfo = cand.transform;
		if (!transInfo->jointMatrices.empty())
		{
			size_t offset = staticDynAlign * staticCount++;
			TransformUbo* currStaticPtr = (TransformUbo*)((uint64_t)staticAlignAlloc.data() + (offset));
			currStaticPtr->modelMatrix = transInfo->modelTransform;

			// the dynamic buffer offsets are stored in the renderable for ease of access when drawing
			rend->staticDynamicOffset = offset;
		}
		else
		{
			size_t offset = skinDynAlign * skinnedCount++;
			SkinnedUbo* currSkinnedPtr =
			    (SkinnedUbo*)((uint64_t)skinAlignAlloc.data() + (skinDynAlign * skinnedCount++));
			currSkinnedPtr->modelMatrix = transInfo->modelTransform;

			// rather than throw an error, clamp the joint if it exceeds the max
			size_t jointCount = std::min(MAX_BONE_COUNT, transInfo->jointMatrices.size());
			memcpy(currSkinnedPtr->jointMatrices, transInfo->jointMatrices.data(), jointCount * sizeof(OEMaths::mat4f));
			rend->skinnedDynamicOffset = offset;
		}
	}

	// Static and skinned model buffers have three outcomes - new instances are created if this is the first frame, the new data is mapped to the existing buffer
	// if the data sisze is within the current buffer size or the buffer is destroyed and a new one created
	// static buffer
	if (staticModelCount)
	{
		size_t staticDataSize = staticModelCount * sizeof(TransformUbo);
		driver.addUbo("StaticTransform", staticDataSize, );
		driver.updateUniform("StaticTransform", staticDataSize, staticAlignAlloc.data());
	}

	// skinned buffer
	if (skinnedModelCount)
	{
		size_t skinnedDataSize = skinnedModelCount * sizeof(SkinnedUbo);
		driver.addUbo("SkinnedTransform", skinnedDataSize, );
		driver.updateUniform("SkinnedTransform", skinnedDataSize, skinAlignAlloc.data());
	}
}

void Scene::updateLightBuffer(std::vector<LightBase*> candLights)
{
	// a mirror of the shader structs
	struct PointLightUbo
	{
		OEMaths::mat4f lightMvp;
		OEMaths::vec4f position;
		OEMaths::colour4 colour;    //< rgb, intensity (lumens)
		float fallOut;
	};

	struct SpotLightUbo
	{
		OEMaths::mat4f lightMvp;
		OEMaths::vec4f position;
		OEMaths::vec4f direction;
		OEMaths::colour4 colour;    //< rgb, intensity (lumens)
		float scale;
		float offset;
		float fallOut;
	};

	struct DirectionalLightUbo
	{
		OEMaths::mat4f lightMvp;
		OEMaths::vec4f position;
		OEMaths::vec4f direction;
		OEMaths::colour4 colour;    //< rgb, intensity (lumens)
	};


	uint32_t spotlightCount = 0;
	uint32_t pointlightCount = 0;
	uint32_t dirLightCount = 0;

	std::array<SpotLightUbo, MAX_SPOT_LIGHTS> spotLights;
	std::array<PointLightUbo, MAX_POINT_LIGHTS> pointLights;
	std::array<DirectionalLightUbo, MAX_DIR_LIGHTS> dirLights;

	// copy the light attributes we need for use in the light shaders.

	for (LightBase* light : candLights)
	{
		if (light->type == LightType::Spot)
		{
			const auto& spotLight = static_cast<SpotLight*>(light);

			// fill in the data to be sent to the gpu
			SpotLightUbo ubo{ light->lightMvp,
				              OEMaths::vec4f{ spotLight->position, 1.0f },
				              OEMaths::vec4f{ spotLight->target, 1.0f },
				              { spotLight->colour, spotLight->intensity },
				              spotLight->scale,
				              spotLight->offset,
				              spotLight->fallout };
			spotLights[spotlightCount++] = ubo;
		}
		else if (light->type == LightType::Point)
		{
			const auto& pointLight = static_cast<PointLight*>(light);

			// fill in the data to be sent to the gpu
			PointLightUbo ubo{ light->lightMvp,
				               OEMaths::vec4f{ pointLight->position, 1.0f },
				               { pointLight->colour, pointLight->intensity },
				               pointLight->fallOut };
			pointLights[pointlightCount++] = ubo;
		}
		else if (light->type == LightType::Directional)
		{
			const auto& dirLight = static_cast<DirectionalLight*>(light);

			// fill in the data to be sent to the gpu
			DirectionalLightUbo ubo{ dirLight->lightMvp,
				                     OEMaths::vec4f{ dirLight->position, 1.0f },
				                     OEMaths::vec4f{ dirLight->target, 1.0f },
				                     { dirLight->colour, dirLight->intensity } };
			dirLights[dirLightCount++] = ubo;
		}
	}

	if (spotlightCount)
	{
		size_t dataSize = spotlightCount * sizeof(SpotLightUbo);
		driver.addUbo("Spotlights", dataSize, );
		driver.updateUniform("SpotLights", dataSize, spotLights.data());
	}
	if (pointlightCount)
	{
		size_t dataSize = pointlightCount * sizeof(PointLightUbo);
		driver.addUbo("Pointlights", dataSize, );
		driver.updateUniform("PointLights", dataSize, pointLights.data());
	}
	if (dirLightCount)
	{
		size_t dataSize = dirLightCount * sizeof(DirectionalLightUbo);
		driver.addUbo("Dirlights", dataSize, );
		driver.updateUniform("DirLights", dataSize, dirLights.data());
	}
}

Camera* Scene::getCurrentCamera()
{
	return &cameras[currCamera];
}

}    // namespace OmegaEngine
