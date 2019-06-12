#include "World.h"
#include "Utility/logger.h"
#include "Utility/FileUtil.h"
#include "Engine/Omega_SceneParser.h"
#include "Engine/Omega_Config.h"
#include "Engine/Omega_Global.h"
#include "ObjectInterface/Object.h"
#include "ObjectInterface/ComponentInterface.h"
#include "ObjectInterface/ObjectManager.h"
#include "ObjectInterface/ComponentTypes.h"
#include "Managers/LightManager.h"
#include "Managers/MeshManager.h"
#include "Managers/TextureManager.h"
#include "Managers/AnimationManager.h"
#include "Managers/TransformManager.h"
#include "Managers/MaterialManager.h"
#include "Managers/TextureManager.h"
#include "Managers/CameraManager.h"
#include "Managers/EventManager.h"
#include "AssetInterface/AssetManager.h"
#include "Rendering/RenderInterface.h"
#include "Models/ModelMaterial.h"
#include "Models/ModelAnimation.h"
#include "Models/GltfModel.h"
#include "Utility/Bvh.hpp"
#include "Vulkan/Device.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT

// tiny gltf uses window.h - this stops the macros messing with std::min
#define NOMINMAX

#include "tiny_gltf.h"

namespace OmegaEngine
{
	World::World()
	{

	}

	World::World(Managers managers, std::unique_ptr<VulkanAPI::Device>& device, EngineConfig& engineConfig)
	{
		// all the boiler plater needed to generate the manager and interface instances
		objectManager = std::make_unique<ObjectManager>();
		componentInterface = std::make_unique<ComponentInterface>();
		renderInterface = std::make_unique<RenderInterface>(device, engineConfig.screenWidth, engineConfig.screenHeight, static_cast<SceneType>(engineConfig.sceneType));
		animation_manager = std::make_unique<AnimationManager>();
		assetManager = std::make_unique<AssetManager>();
		bvh = std::make_unique<BVH>();

		// register all components managers required for this world
		if (managers & Managers::OE_MANAGERS_MESH || managers & Managers::OE_MANAGERS_ALL)
		{
			componentInterface->registerManager<MeshManager>();
		}
		if (managers & Managers::OE_MANAGERS_MATERIAL || managers & Managers::OE_MANAGERS_ALL) 
		{
			componentInterface->registerManager<MaterialManager>();
		}
		if (managers & Managers::OE_MANAGERS_TEXTURE || managers & Managers::OE_MANAGERS_ALL) 
		{
			componentInterface->registerManager<TextureManager>();
		}
		if (managers & Managers::OE_MANAGERS_LIGHT || managers & Managers::OE_MANAGERS_ALL) 
		{
			componentInterface->registerManager<LightManager>();
		}
		if (managers & Managers::OE_MANAGERS_TRANSFORM || managers & Managers::OE_MANAGERS_ALL)
		{
			componentInterface->registerManager<TransformManager>();
		}
		if (managers & Managers::OE_MANAGERS_CAMERA || managers & Managers::OE_MANAGERS_ALL) 
		{
			componentInterface->registerManager<CameraManager>(engineConfig.mouseSensitivity);
		}
		
		// setup the preferred renderer and associated elements
		renderInterface->initRenderer(componentInterface);
	}

	World::~World()
	{
		
	}

	bool World::create(const char* filename, const char* name)
	{
		// this worlds name, used as a reference
		std::strcpy(this->name, name);

		SceneParser parser;
		if (!parser.parse(filename))
		{
			return false;
		}

		// update camera manager 
		componentInterface->getManager<CameraManager>().addCamera(parser.getCamera());

		// add lights from scene file
		for (uint32_t i = 0; i < parser.lightCount(); ++i) 
		{
			componentInterface->getManager<LightManager>().addLight(parser.getLights(i));
		}

		// environment - load skybox and IBL files into memory if they exsist
		if (parser.getEnvironment().skyboxFilename)
		{
			assetManager->loadImageFile(parser.getEnvironment().skyboxFilename);

			// add skybox as a object - TODO: blur factor should be obtained from the config settings 
			auto obj = &objectManager->createObject();
			obj->addComponent<SkyboxComponent>(1.0f);
		}
		if (parser.getEnvironment().brdfFilename)
		{
			assetManager->loadImageFile(parser.getEnvironment().brdfFilename);
		}
		if (parser.getEnvironment().irradianceMapFilename)
		{
			assetManager->loadImageFile(parser.getEnvironment().irradianceMapFilename);
		}

		
		return true;
	}

	void World::create(const char *name)
	{
		// this worlds name, used as a reference
		std::strcpy(this->name, name);

		// an empty world, so not much to do for now!
	}

	Object* World::createObject()
	{
		 auto object = objectManager->createObject();

		 // add object to queue for updating its components with the relevant managers
		 componentInterface->addObjectToUpdateQueue(object);
		 return object;
	}

	Object *World::createGltfModelObjectRecursive(std::unique_ptr<ModelNode>& node, Object* parentObject, 
		const uint32_t materialOffset, const uint32_t skinOffset, const uint32_t animationOffset)
	{
		if (node->hasChildren)
		{
			for (uint32_t i = 0; i < node->childCount(); ++i)
			{
				auto child = objectManager->createChildObject(*parentObject);
				this->createGltfModelObjectRecursive(node->getChildNode(i), child, materialOffset, skinOffset);
			}
		}

		if (node->hasMesh())
		{
			parentObject->addComponent<MeshComponent>(node->getMesh(), materialOffset);
		}
		if (node->hasTransform())
		{
			parentObject->addComponent<TransformComponent>(node->getTransform());
		}
		if (node->hasSkin())
		{
			parentObject->addComponent<SkinnedComponent>(node->getSkinIndex(), skinOffset, node->isSkeletonRoot(), node->isJoint());
		}
		if (node->hasAnimation())
		{
			parentObject->addComponent<AnimationComponent>(node->getAnimIndex(), node->getChannelIndex(), animationOffset);
		}
	}

	Object* World::createGltfModelObject(std::unique_ptr<GltfModel::Model>& model, bool useMaterial)
	{
		// materials and their textures
		auto& materialManager = componentInterface->getManager<MaterialManager>();
		uint32_t materialOffset = materialManager.getBufferOffset();

		for (auto& material : model->materials)
		{
			materialManager.addMaterial(material, model->images, assetManager);
		}

		// skins
		auto& transformManager = componentInterface->getManager<TransformManager>();
		uint32_t skinOffset = transformManager.getSkinnedBufferOffset();

		for (auto& skin : model->skins)
		{
			transformManager.addSkin(skin);
		}

		// animations
		auto& animationManager = componentInterface->getManager<AnimationManager>();
		uint32_t animationOffset = animationManager.getBufferOffset();

		for (auto& animation : model->animations)
		{
			animationManager.addAnimation(animation);
		}

		// Now to create the object and add the relevant components
		auto object = objectManager->createObject();

		for (auto& node : model->nodes)
		{
			this->createGltfModelObjectRecursive(node, object, materialOffset, skinOffset, animationOffset);
		}

		return object;
	}

	void World::update(double time, double dt)
	{
		// update on a per-frame basis
		// animation
		animation_manager->updateAnimation(time, dt, componentInterface->getManager<TransformManager>());

		// newly added assets need to be hosted on the gpu
		assetManager->update();

		// all other managers
		componentInterface->updateManagers(time, dt, objectManager);

		// check whether there are any queued events to deal with
		Global::eventManager()->notifyQueued();

		// add all objects as renderable targets unless they flagged otherwise 
		renderInterface->updateRenderables(objectManager, componentInterface);

		hasUpdatedOnce = true;
	}

	void World::render(double interpolation)
	{
		if (hasUpdatedOnce) 
		{
			renderInterface->render(interpolation);
		}
	}


	

}