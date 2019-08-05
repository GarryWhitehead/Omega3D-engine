#include "World.h"
#include "AssetInterface/AssetManager.h"
#include "Engine/Omega_Config.h"
#include "Engine/Omega_Global.h"
#include "Engine/Omega_SceneParser.h"
#include "Managers/AnimationManager.h"
#include "Managers/EventManager.h"
#include "Managers/MaterialManager.h"
#include "Managers/MeshManager.h"
#include "Managers/TransformManager.h"
#include "Models/Gltf/GltfModel.h"
#include "Models/ModelAnimation.h"
#include "Models/ModelMaterial.h"
#include "ObjectInterface/ComponentInterface.h"
#include "ObjectInterface/ComponentTypes.h"
#include "ObjectInterface/Object.h"
#include "ObjectInterface/ObjectManager.h"
#include "Rendering/RenderInterface.h"
#include "Utility/Bvh.hpp"
#include "Utility/FileUtil.h"
#include "Utility/logger.h"
#include "VulkanAPI/Device.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT

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
	renderInterface = std::make_unique<RenderInterface>(device, engineConfig.screenWidth, engineConfig.screenHeight,
	                                                    static_cast<SceneType>(engineConfig.sceneType));
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
	if (managers & Managers::OE_MANAGERS_LIGHT || managers & Managers::OE_MANAGERS_ALL)
	{
		componentInterface->registerManager<LightManager>();
	}
	if (managers & Managers::OE_MANAGERS_TRANSFORM || managers & Managers::OE_MANAGERS_ALL)
	{
		componentInterface->registerManager<TransformManager>();
	}
	if (managers & Managers::OE_MANAGERS_ANIMATION || managers & Managers::OE_MANAGERS_ALL)
	{
		componentInterface->registerManager<AnimationManager>();
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

bool World::create(const std::string& filename, const std::string& name)
{
	// this worlds name, used as a reference
	this->name = name;

	SceneParser parser;
	if (!parser.parse(filename))
	{
		return false;
	}

	// TODO: NEEDS UPDATING!
	/*
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
		}*/

	return true;
}

void World::create(const std::string& name)
{
	// this worlds name, used as a reference
	this->name = name;

	// an empty world, so not much to do for now!
}

Object* World::createObject(const OEMaths::vec3f& position, const OEMaths::vec3f& scale, const OEMaths::quatf& rotation)
{
	auto object = objectManager->createObject();

	// all root objects have the world transform
	object->addComponent<WorldTransformComponent>(position, scale, rotation);

	// add object to queue for updating its components with the relevant managers
	componentInterface->addObjectToUpdateQueue(object);

	return object;
}

void World::extractGltfModelAssets(std::unique_ptr<GltfModel::Model>& model, uint32_t& materialOffset,
                                   uint32_t& skinOffset, uint32_t& animationOffset)
{
	// materials and their textures
	auto& materialManager = componentInterface->getManager<MaterialManager>();
	materialOffset = materialManager.getBufferOffset();

	for (auto& material : model->materials)
	{
		materialManager.addMaterial(material, model->images);
	}

	// skins
	auto& transformManager = componentInterface->getManager<TransformManager>();
	skinOffset = transformManager.getSkinnedBufferOffset();

	for (auto& skin : model->skins)
	{
		transformManager.addSkin(skin);
	}

	// animations
	auto& animationManager = componentInterface->getManager<AnimationManager>();
	animationOffset = animationManager.getBufferOffset();

	for (auto& animation : model->animations)
	{
		animationManager.addAnimation(animation);
	}
}

void World::createGltfModelObjectRecursive(std::unique_ptr<GltfModel::ModelNode>& node, Object* parentObject,
                                           const uint32_t materialOffset, const uint32_t skinOffset,
                                           const uint32_t animationOffset)
{
	if (node->hasMesh())
	{
		parentObject->addComponent<MeshComponent>(node->getMesh(), materialOffset);
		// TODO: obtain these parameters from the config once it has been refactored
		parentObject->addComponent<ShadowComponent>(0.0f, 1.25f, 1.75f);
	}
	if (node->hasTransform())
	{
		parentObject->addComponent<TransformComponent>(node->getTransform());
	}
	if (node->hasSkin())
	{
		parentObject->addComponent<SkinnedComponent>(node->getSkinIndex(), skinOffset);
	}
	if (node->isJoint())
	{
		parentObject->addComponent<SkeletonComponent>(node->getJoint(), skinOffset, node->isSkeletonRoot());
	}
	if (node->hasAnimation())
	{
		parentObject->addComponent<AnimationComponent>(node->getAnimIndex(), node->getChannelIndices(),
		                                               animationOffset);
	}

	if (node->hasChildren())
	{
		for (uint32_t i = 0; i < node->childCount(); ++i)
		{
			auto child = objectManager->createChildObject(*parentObject);
			this->createGltfModelObjectRecursive(node->getChildNode(i), child, materialOffset, skinOffset,
			                                     animationOffset);
		}
	}
}

Object* World::createGltfModelObject(std::unique_ptr<GltfModel::Model>& model, const OEMaths::vec3f& position,
                                     const OEMaths::vec3f& scale, const OEMaths::quatf& rotation, bool useMaterial)
{
	uint32_t materialOffset, skinOffset, animationOffset;

	extractGltfModelAssets(model, materialOffset, skinOffset, animationOffset);

	// Now to create the object and add the relevant components
	// The layout of objects for gltf models is that the root contains the world transform and
	// subsequent child objects contain the nodes. Allows models to have multiple nodes but
	// be transformed by the root world matrix
	auto rootObject = this->createObject(position, scale, rotation);

	for (auto& node : model->nodes)
	{
		auto child = objectManager->createChildObject(*rootObject);

		this->createGltfModelObjectRecursive(node, child, materialOffset, skinOffset, animationOffset);
	}

	return rootObject;
}

// TODO : The world really shouldn't have this boiler plate. These should be dealt with as components
void World::addSkybox(const std::string& filename, float blurFactor)
{
	assetManager->loadImageFile(filename, "Skybox");
	auto object = objectManager->createObject();
	object->addComponent<SkyboxComponent>(blurFactor);
}

void World::addCameraToWorld(OEMaths::vec3f& startPosition, float fov, float zNear, float zFar, float aspect,
                             float velocity, Camera::CameraType type)
{
	auto& cameraManager = componentInterface->getManager<CameraManager>();
	cameraManager.addCamera(startPosition, fov, zNear, zFar, aspect, velocity, type);
}

void World::addSpotLightToWorld(const OEMaths::vec3f& position, const OEMaths::vec3f& target,
                                const OEMaths::vec3f& colour, float fov, float intensity, float fallOut,
                                float innerCone, float outerCone, const LightAnimateType animType, float animVel)
{
	auto& lightManager = componentInterface->getManager<LightManager>();
	lightManager.addSpotLight(position, target, colour, fov, intensity, fallOut, innerCone, outerCone, animType,
	                          animVel);
}

void World::addPointLightToWorld(const OEMaths::vec3f& position, const OEMaths::vec3f& target,
                                 const OEMaths::vec3f& colour, float fov, float intensity, float fallOut,
                                 const LightAnimateType animType, float animVel)
{
	auto& lightManager = componentInterface->getManager<LightManager>();
	lightManager.addPointLight(position, target, colour, fov, intensity, fallOut, animType, animVel);
}

void World::update(double time, double dt)
{
	// update on a per-frame basis

	// all other managers
	componentInterface->update(time, dt, objectManager);

	// newly added assets need to be hosted on the gpu
	assetManager->update(componentInterface);

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

}    // namespace OmegaEngine
