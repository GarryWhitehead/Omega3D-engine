#include "World.h"

#include "Utility/logger.h"

#include "Core/Omega_Global.h"
#include "Core/Scene.h"

#include "Managers/EventManager.h"

namespace OmegaEngine
{
World::World()
{
}

World::World()
{
}

World::~World()
{
}

void World::prepare(const std::string& name)
{
	// this worlds name, used as a reference
	this->name = name;

	// an empty world, so not much to do for now!
}

Scene* World::createScene()
{
	Scene scene(*this);
	scenes.push_back(scene);
	return &scenes.back();
}

void World::update(double time, double dt)
{
	// update on a per-frame basis
	// all other managers
	animManager.updateFrame(time, *this);
	cameraManager.updateFrame(time, dt);
	lightManager.updateFrame(dt, *this);
	meshManager.updateFrame();
	transManager.updateFrame(objManager);

	// newly added assets need to be hosted on the gpu
	assetManager.update();

	// check whether there are any queued events to deal with
	Global::eventManager()->notifyQueued();
}

// ** manager helper functions **
AnimationManager& World::getAnimManager()
{
	return animManager;
}

CameraManager& World::getCameraManager()
{
	return cameraManager;
}

LightManager& World::getLightManager()
{
	return lightManager;
}

MaterialManager& World::getMatManager()
{
	return matManager;
}

MeshManager& World::getMeshManager()
{
	return meshManager;
}

TransformManager& World::getTransManager()
{
	return transManager;
}

AssetManager& World::getAssetManager()
{
	return assetManager;
}

}    // namespace OmegaEngine
