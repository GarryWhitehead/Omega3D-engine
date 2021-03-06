#include "RenderInterface.h"
#include "Engine/Omega_Global.h"
#include "Engine/World.h"
#include "Managers/CameraManager.h"
#include "Managers/LightManager.h"
#include "Managers/MaterialManager.h"
#include "Managers/MeshManager.h"
#include "Managers/TransformManager.h"
#include "ObjectInterface/ComponentInterface.h"
#include "ObjectInterface/ComponentTypes.h"
#include "ObjectInterface/Object.h"
#include "ObjectInterface/ObjectManager.h"
#include "PostProcess/PostProcessInterface.h"
#include "RenderableTypes/Mesh.h"
#include "RenderableTypes/RenderableBase.h"
#include "RenderableTypes/Shadow.h"
#include "RenderableTypes/Skybox.h"
#include "Rendering/ProgramStateManager.h"
#include "Rendering/RenderQueue.h"
#include "Rendering/Renderers/DeferredRenderer.h"
#include "Threading/ThreadPool.h"
#include "Utility/FileUtil.h"
#include "Utility/logger.h"
#include "VulkanAPI/Device.h"
#include "VulkanAPI/Interface.h"
#include "VulkanAPI/VkTextureManager.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"

namespace OmegaEngine
{
RenderInterface::RenderInterface()
{
}

RenderInterface::RenderInterface(std::unique_ptr<VulkanAPI::Device>& device, const uint32_t width,
                                 const uint32_t height, SceneType type)
    : sceneType(type)
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

	// and the state manager which will deal with shader modules and pipelines
	stateManager = std::make_unique<ProgramStateManager>();
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
		LOGGER_ERROR("Using a unsupported rendering pipeline. At the moment only deferred shader "
		             "is supported.");
		break;
	}
}

void RenderInterface::buildRenderableMeshTree(Object& obj, std::unique_ptr<ComponentInterface>& componentInterface,
                                              bool isShadow)
{
	auto& meshManager = componentInterface->getManager<MeshManager>();
	auto& lightManager = componentInterface->getManager<LightManager>();

	if (obj.hasComponent<MeshComponent>())
	{
		auto& mesh = meshManager.getMesh(obj.getComponent<MeshComponent>());

		// we need to add all the primitve sub meshes as renderables
		for (auto& primitive : mesh.primitives)
		{
			uint32_t meshIndex = addRenderable<RenderableMesh>(componentInterface, vkInterface, mesh, primitive, obj,
			                                                   stateManager, renderer);

			// if using shadows, then draw the meshes into the offscreen depth buffer too
			if (obj.hasComponent<ShadowComponent>())
			{
				addRenderable<RenderableShadow>(stateManager, vkInterface, obj.getComponent<ShadowComponent>(), mesh,
				                                primitive, lightManager.getLightCount(),
				                                lightManager.getAlignmentSize(), renderer);
			}
		}
	}

	// check whether the child objects contain mesh data
	auto children = obj.getChildren();
	for (auto child : children)
	{
		buildRenderableMeshTree(child, componentInterface, isShadow);
	}
}

void RenderInterface::updateRenderables(std::unique_ptr<ObjectManager>& objectManager,
                                        std::unique_ptr<ComponentInterface>& componentInterface)
{
	if (isDirty)
	{
		// get all objects that are available this frame
		auto& objects = objectManager->getObjectsList();

		for (auto& object : objects)
		{
			// objects which have skybox, landscape or ocean components won't have other components checked
			if (object.second.hasComponent<SkyboxComponent>())
			{
				addRenderable<RenderableSkybox>(stateManager, object.second.getComponent<SkyboxComponent>(),
				                                vkInterface, renderer);
			}
			else
			{
				// check whether object or children have mesh data
				buildRenderableMeshTree(object.second, componentInterface, false);
			}
		}

		isDirty = false;
	}
}

void RenderInterface::prepareObjectQueue()
{
	RenderQueueInfo queueInfo;

	for (auto& info : renderables)
	{

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
}    // namespace OmegaEngine
