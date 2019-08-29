#include "World.h"
#include "AssetInterface/AssetManager.h"
#include "Engine/Omega_Config.h"
#include "Engine/Omega_Global.h"
#include "Engine/Omega_SceneParser.h"
#include "Managers/TransformManager.h"
#include "Models/Gltf/GltfModel.h"
#include "Models/ModelAnimation.h"
#include "Models/ModelMaterial.h"
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

void World::addDirectionalLightToWorld(const OEMaths::vec3f& position, const OEMaths::vec3f& target,
                                 const OEMaths::vec3f& colour, float fov, float intensity)
{
	auto& lightManager = componentInterface->getManager<LightManager>();
	lightManager.addDirectionalLight(position, target, colour, fov, intensity);
}

void World::update(double time, double dt)
{
	// update on a per-frame basis

	// all other managers
	componentInterface->update(time, dt, objectManager);

	// newly added assets need to be hosted on the gpu
	assetManager.update();

	// check whether there are any queued events to deal with
	Global::eventManager()->notifyQueued();

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
