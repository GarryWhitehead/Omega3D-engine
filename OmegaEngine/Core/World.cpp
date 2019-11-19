#include "World.h"

#include "Components/AnimationManager.h"
#include "Components/CameraManager.h"
#include "Components/LightManager.h"
#include "Components/RenderableManager.h"
#include "Components/TransformManager.h"

#include "Utility/logger.h"

#include "Core/Omega_Global.h"
#include "Core/Scene.h"

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
	animManager->updateFrame(time, *this);
	cameraManager->updateFrame(time, dt);
	lightManager->updateFrame(dt, *this);
	rendManager->updateFrame();
	transManager->updateFrame(objManager);

	// check whether there are any queued events to deal with
	Global::eventManager()->notifyQueued();
}

}    // namespace OmegaEngine
