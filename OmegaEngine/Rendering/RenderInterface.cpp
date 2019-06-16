#include "RenderInterface.h"
#include "RenderableTypes/RenderableBase.h"
#include "RenderableTypes/Mesh.h"
#include "RenderableTypes/Skybox.h"
#include "RenderableTypes/Shadow.h"
#include "Rendering/RenderQueue.h"
#include "Rendering/Renderers/DeferredRenderer.h"
#include "ObjectInterface/ComponentInterface.h"
#include "ObjectInterface/ComponentTypes.h"
#include "PostProcess/PostProcessInterface.h"
#include "ObjectInterface/Object.h"
#include "ObjectInterface/ObjectManager.h"
#include "Managers/TransformManager.h"
#include "Managers/CameraManager.h"
#include "Managers/MeshManager.h"
#include "Managers/MaterialManager.h"
#include "Managers/LightManager.h"
#include "Utility/logger.h"
#include "Utility/FileUtil.h"
#include "Threading/ThreadPool.h"
#include "Engine/Omega_Global.h"
#include "Engine/World.h"
#include "VulkanAPI/VkTextureManager.h"
#include "VulkanAPI/Device.h"
#include "VulkanAPI/Interface.h"

#include "rapidjson/stringbuffer.h"
#include "rapidjson/document.h"


namespace OmegaEngine
{
	RenderInterface::RenderInterface()
	{
	}

	RenderInterface::RenderInterface(std::unique_ptr<VulkanAPI::Device>& device, const uint32_t width, const uint32_t height, SceneType type) :
		sceneType(type)
	{
		init(device, width, height);
	}


	RenderInterface::~RenderInterface()
	{
	}

	void RenderInterface::init(std::unique_ptr<VulkanAPI::Device>& device, const uint32_t width, const uint32_t height)
	{
		// load the render config file if it exsists
		// renderConfig.load();

		// all renderable elements will be dispatched for drawing via this queue
		renderQueue = std::make_unique<RenderQueue>();

		// initiliase the graphical backend - we are solely using Vulkan
		// The new frame mode depends on tehe scene type - static scenes will only have their cmd buffers recorded
		// to once whilst dynamic scenes will be recorded to on a per frame basis.
		VulkanAPI::NewFrameMode mode;
		if (sceneType == SceneType::Static)
		{
			mode = VulkanAPI::NewFrameMode::Static;
		}
		else 
		{
			// this could be either the renewal or resetting of cmd buffers - what;'s best needs to be checked
			mode = VulkanAPI::NewFrameMode::Reset;
		}

		// create the vulkan API interface - this is the middle man between the renderer and the vulkan backend
		vkInterface = std::make_unique<VulkanAPI::Interface>(*device, width, height, mode);

		// also add stock model vertices/indices if required
		if (renderConfig.general.useStockModels)
		{
			cubeModel = std::make_unique<RenderUtil::CubeModel>();
			planeModel = std::make_unique<RenderUtil::PlaneModel>();
		}
	}

	void RenderInterface::initRenderer(std::unique_ptr<ComponentInterface>& componentInterface)
	{
		// setup the renderer pipeline
		switch (static_cast<RendererType>(renderConfig.general.renderer)) 
		{
			case RendererType::Deferred:
			{
				setRenderer<DeferredRenderer>(*vkInterface, renderConfig);
				break;
			}
			default:
				LOGGER_ERROR("Using a unsupported rendering pipeline. At the moment only deferred shader is supported.");
				break;
		}

		// initlaise all shaders and pipelines that will be used which is dependent on the number of renderable types
		for (uint16_t r_type = 0; r_type < (uint16_t)RenderTypes::Count; ++r_type) 
		{
			this->addShader((RenderTypes)r_type, componentInterface);
		}
	}
	
	void RenderInterface::addShader(RenderTypes type, std::unique_ptr<ComponentInterface>& componentInterface)
	{
		auto& state = std::make_unique<ProgramState>();

		switch (type) 
		{
			case OmegaEngine::RenderTypes::StaticMesh:
			{
				 RenderableMesh::createMeshPipeline(vkInterface->getDevice(),
					renderer, vkInterface->getBufferManager(), vkInterface->gettextureManager(), MeshManager::MeshType::Static, state);
				 renderStates[(int)RenderTypes::StaticMesh] = std::move(state);
				break;
			}
			case OmegaEngine::RenderTypes::SkinnedMesh: 
			{
				RenderableMesh::createMeshPipeline(vkInterface->getDevice(),
					renderer, vkInterface->getBufferManager(), vkInterface->gettextureManager(), MeshManager::MeshType::Skinned, state);
				renderStates[(int)RenderTypes::SkinnedMesh] = std::move(state);
				break;
			}
			case OmegaEngine::RenderTypes::ShadowMapped:
			{
				RenderableShadow::createShadowPipeline(vkInterface->getDevice(), renderer, vkInterface->getBufferManager(), state);
				renderStates[(int)RenderTypes::ShadowMapped] = std::move(state);
				break;
			}
			case OmegaEngine::RenderTypes::Skybox: 
			{
				RenderableSkybox::createSkyboxPipeline(vkInterface->getDevice(),
					renderer, vkInterface->getBufferManager(), vkInterface->gettextureManager(), state);
				renderStates[(int)RenderTypes::Skybox] = std::move(state);
				break;
			}
			default:
				LOGGER_INFO("Unsupported render type found whilst initilaising shaders.");
		}
	}

	void RenderInterface::buildRenderableMeshTree(Object& obj, std::unique_ptr<ComponentInterface>& componentInterface, bool isShadow)
	{
		auto& meshManager = componentInterface->getManager<MeshManager>();
		auto& lightManager = componentInterface->getManager<LightManager>();
	
		auto& mesh = meshManager.getMesh(obj.getComponent<MeshComponent>());

		// we need to add all the primitve sub meshes as renderables
		for (auto& primitive : mesh.primitives) 
		{
			uint32_t meshIndex = addRenderable<RenderableMesh>(vkInterface->getDevice(), componentInterface, vkInterface->getBufferManager(),
				vkInterface->gettextureManager(), mesh, primitive, obj, this);
			
			// if using shadows, then draw the meshes into the offscreen depth buffer too
			// TODO : this should be done elsewhere and make this code better!
			obj.addComponent<ShadowComponent>(renderConfig.biasClamp, renderConfig.biasConstant, renderConfig.biasSlope);

			addRenderable<RenderableShadow>(this, obj.getComponent<ShadowComponent>(), 
				getRenderable(meshIndex).renderable->getInstanceData<RenderableMesh::MeshInstance>(), lightManager.getLightCount(), lightManager.getAlignmentSize());
		}
	
		// and do the same for all children associated with this mesh
		auto children = obj.getChildren();
		for (auto child : children) 
		{
			buildRenderableMeshTree(child, componentInterface, isShadow);
		}
	}

	void RenderInterface::updateRenderables(std::unique_ptr<ObjectManager>& objectManager, std::unique_ptr<ComponentInterface>& componentInterface)
	{
		if (isDirty)
		{
			// get all objects that are available this frame
			auto& objects = objectManager->getObjectsList();

			for (auto& object : objects) 
			{
				if (object.second.hasComponent<MeshComponent>())
				{
					buildRenderableMeshTree(object.second, componentInterface, false);
				}
				if (object.second.hasComponent<SkyboxComponent>())
				{
					addRenderable<RenderableSkybox>(this, object.second.getComponent<SkyboxComponent>(), vkInterface->getBufferManager());
				}
			}

			isDirty = false;
		}
	}

	void RenderInterface::prepareObjectQueue()
	{
		RenderQueueInfo queueInfo;

		for (auto& info : renderables) {

			switch (info.renderable->getRenderType()) 
			{
				case RenderTypes::SkinnedMesh:
				case RenderTypes::StaticMesh: 
				{
					queueInfo.renderableHandle = info.renderable->getHandle();
					queueInfo.renderFunction = getMemberRenderFunction<void, RenderableMesh, &RenderableMesh::render>;
					break;
				}
				case RenderTypes::ShadowMapped: 
				{
					queueInfo.renderableHandle = info.renderable->getHandle();
					queueInfo.renderFunction = getMemberRenderFunction<void, RenderableShadow, &RenderableShadow::render>;
					break;
				}
				case RenderTypes::Skybox: 
				{
					queueInfo.renderableHandle = info.renderable->getHandle();
					queueInfo.renderFunction = getMemberRenderFunction<void, RenderableSkybox, &RenderableSkybox::render>;
					break;
				}
			}

			queueInfo.renderableData = info.renderable->getInstanceData();
			queueInfo.sortingKey = info.renderable->getSortKey();
			queueInfo.queueType = info.renderable->getQueueType();

			renderQueue->addRenderableToQueue(queueInfo);
		}
	}

	void RenderInterface::render(double interpolation)
	{
		// update buffer and texture descriptors before doing the rendering
		vkInterface->getBufferManager()->update();
		vkInterface->gettextureManager()->update();

		// add the renderables to the queue
		// TODO: add visibility check
		prepareObjectQueue();
		
		renderer->render(vkInterface, sceneType, renderQueue);
	}
}
