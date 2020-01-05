#include "Scene.h"

#include "Types/Object.h"
#include "Types/Skybox.h"

#include "Core/Camera.h"
#include "Core/Frustum.h"
#include "Core/World.h"
#include "Core/Engine.h"

#include "Threading/ThreadPool.h"

#include "Components/LightManager.h"
#include "Components/RenderableManager.h"
#include "Components/TransformManager.h"

#include "Rendering/GBufferFillPass.h"

#include "utility/AlignedAlloc.h"

#include "VulkanAPI/VkDriver.h"

namespace OmegaEngine
{

Scene::Scene(World& world, Engine& engine, VulkanAPI::VkDriver& driver)
    : driver(driver),
        world(world)
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
    driver.addUbo("CameraUbo", sizeof(Camera::Ubo), VulkanAPI::Buffer::Usage::Dynamic);
}

void Scene::getVisibleRenderables(Frustum& frustum, std::vector<Scene::VisibleCandidate>& renderables)
{
	size_t workSize = renderables.size();

	auto visibilityCheck = [&frustum, &renderables](size_t curr_idx, size_t chunkSize) {
		assert(curr_idx + chunkSize < renderables.size());
		for (size_t idx = curr_idx; idx < curr_idx + chunkSize; ++idx)
		{
			Renderable* rend = renderables[idx].renderable;
            AABBox& box = rend->instance->getAABBox();
			if (frustum.checkBoxPlaneIntersect(box))
			{
				rend->visibility |= Renderable::Visible::Render;
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

Scene::VisibleCandidate Scene::buildRendCandidate(Object* obj, OEMaths::mat4f& worldMat)
{
    auto& transManager = engine.getTransManager();
    auto& rendManager = engine.getRendManager();
    
    const ObjHandle tHandle = transManager.getObjIndex(*obj);
    const ObjHandle rHandle = rendManager.getObjIndex(*obj);
    
    VisibleCandidate candidate;
    candidate.renderable = &rendManager.getMesh(rHandle);
    candidate.transform = &transManager.getTransform(tHandle);

    // calculate the world-orientated AABB
    OEMaths::mat4f localMat = candidate.transform->modelTransform;
    candidate.worldTransform = worldMat * localMat;
    
    candidate.worldAABB = AABBox::calculateRigidTransform(candidate.renderable->instance->getAABBox(), candidate.worldTransform);
    return candidate;
}

void Scene::update()
{
     auto& objects = world.getObjManager().getObjectsList();
    
    // we create a temp container as we will be doing the visibility checks async
	// reserve more space than we need
	std::vector<VisibleCandidate> candRenderableObjs(objects.size());
    
    // iterate through the model graph add as a possible candidate if active
    for (ModelGraph::Node* node : modelGraph.nodes)
    {
        // if the parent is inactive, then all its children are too
        if (!node->parent->isActive())
        {
            continue;
        }
        
        OEMaths::mat4f worldMat = node->world.worldMat;
        
        // the parent
        VisibleCandidate candidate = buildRendCandidate(node->parent, worldMat);
        candRenderableObjs.emplace_back(candidate);
        
        // and the children if any
        for (Object* child : node->children)
        {
            if (!child->isActive())
            {
                continue;
            }
            
            VisibleCandidate childCand = buildRendCandidate(child, worldMat);
            candRenderableObjs.emplace_back(childCand);
        }
    }
    
	// now for the lights. At the moment we iterate through the list of objects and find any that have a
    // light component. If they are active then these are added as a potential candiate lighting source
    auto& lightManager = engine.getLightManager();
    
    std::vector<LightBase*> candLightObjs(objects.size());
    
	for (Object& obj : objects)
	{
		if (!obj.isActive())
		{
			continue;
		}

        ObjHandle lHandle = lightManager.getObjIndex(obj);
        if (lHandle)
        {
            candLightObjs.emplace_back(lightManager.getLight(lHandle));
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
		if (!rend->visibility.testBit(Renderable::Visible::Render))
		{
			continue;
		}

        if (rend->instance->variantBits.testBit(MeshInstance::Variant::HasSkin))
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
		    RenderQueue::createSortKey(RenderQueue::Layer::Default, rend->materialId, rend->instance->variantBits.getUint64());
		queueRend.emplace_back(queueInfo);
	}
	renderQueue.pushRenderables(queueRend, RenderQueue::Partition::Colour);

	// ================== update ubos =================================
	// camera buffer is updated every frame as we expect this to change a lot
	updateCameraBuffer();

	// we also update the transforms every frame though could have a dirty flag
	updateTransformBuffer(candRenderableObjs, staticModelCount, skinnedModelCount);
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

	driver.updateUbo("CameraUbo", sizeof(Camera::Ubo), &ubo);
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
        OEMaths::mat4f jointMatrices[TransformManager::MAX_BONE_COUNT];
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
		if (!rend->visibility.testBit(Renderable::Visible::Render))
		{
			continue;
		}

		TransformInfo* transInfo = cand.transform;
		if (!transInfo->jointMatrices.empty())
		{
			size_t offset = staticDynAlign * staticCount++;
			TransformUbo* currStaticPtr = (TransformUbo*)((uint64_t)staticAlignAlloc.getData() + (offset));
			currStaticPtr->modelMatrix = transInfo->modelTransform;

			// the dynamic buffer offsets are stored in the renderable for ease of access when drawing
			rend->dynamicOffset = offset;
		}
		else
		{
			size_t offset = skinDynAlign * skinnedCount++;
			SkinnedUbo* currSkinnedPtr =
			    (SkinnedUbo*)((uint64_t)skinAlignAlloc.getData() + (skinDynAlign * skinnedCount++));
			currSkinnedPtr->modelMatrix = cand.worldTransform;

			// rather than throw an error, clamp the joint if it exceeds the max
			uint32_t jointCount = std::min(TransformManager::MAX_BONE_COUNT, static_cast<uint32_t>(transInfo->jointMatrices.size()));
			memcpy(currSkinnedPtr->jointMatrices, transInfo->jointMatrices.data(), jointCount * sizeof(OEMaths::mat4f));
			rend->dynamicOffset = offset;
		}
	}

	// Static and skinned model buffers have three outcomes - new instances are created if this is the first frame, the new data is mapped to the existing buffer
	// if the data sisze is within the current buffer size or the buffer is destroyed and a new one created
	// static buffer
	if (staticModelCount)
	{
		size_t staticDataSize = staticModelCount * sizeof(TransformUbo);
		driver.addUbo("StaticTransform", staticDataSize, VulkanAPI::Buffer::Usage::Static);
		driver.updateUbo("StaticTransform", staticDataSize, staticAlignAlloc.getData());
	}

	// skinned buffer
	if (skinnedModelCount)
	{
		size_t skinnedDataSize = skinnedModelCount * sizeof(SkinnedUbo);
		driver.addUbo("SkinnedTransform", skinnedDataSize, VulkanAPI::Buffer::Usage::Static);
		driver.updateUbo("SkinnedTransform", skinnedDataSize, skinAlignAlloc.getData());
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

	std::array<SpotLightUbo, LightManager::MAX_SPOT_LIGHTS> spotLights;
	std::array<PointLightUbo, LightManager::MAX_POINT_LIGHTS> pointLights;
	std::array<DirectionalLightUbo, LightManager::MAX_DIR_LIGHTS> dirLights;

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
		driver.addUbo("Spotlights", dataSize, VulkanAPI::Buffer::Usage::Dynamic);
		driver.updateUbo("SpotLights", dataSize, spotLights.data());
	}
	if (pointlightCount)
	{
		size_t dataSize = pointlightCount * sizeof(PointLightUbo);
		driver.addUbo("Pointlights", dataSize, VulkanAPI::Buffer::Usage::Dynamic);
		driver.updateUbo("PointLights", dataSize, pointLights.data());
	}
	if (dirLightCount)
	{
		size_t dataSize = dirLightCount * sizeof(DirectionalLightUbo);
		driver.addUbo("Dirlights", dataSize, VulkanAPI::Buffer::Usage::Dynamic);
		driver.updateUbo("DirLights", dataSize, dirLights.data());
	}
}

Camera* Scene::getCurrentCamera()
{
	if (cameras.empty())
    {
        LOGGER_WARN("Trying to retrieve a camera when none have been registered with the scene");
        return nullptr;
    }
    assert(currCamera < cameras.size());
    
    return &cameras[currCamera];
}

bool Scene::addSkybox(SkyboxInstance& instance)
{
    if (!instance.cubeMap)
    {
        LOGGER_WARN("The cubemap enviromental texture is nullptr!");
        return false;
    }
    if (!instance.cubeMap->isCubeMap())
    {
        LOGGER_WARN("The cubemap texture is not a cubemap!");
        return false;
    }
    
    skybox = std::make_unique<Skybox>(driver, instance.cubeMap, instance.blur);
    assert(skybox);
}

}    // namespace OmegaEngine
