#include "World.h"
#include "Utility/logger.h"

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
	return materialManager;
}

MeshManager& World::getMeshManager()
{
	return meshManager;
}

TranformManager& World::getTransManager()
{
	return transformManager;
}

AssetManager& World::getAssetManager()
{
	return assetManager;
}

}    // namespace OmegaEngine
