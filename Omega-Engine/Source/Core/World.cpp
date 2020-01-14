#include "World.h"

#include "Components/AnimationManager.h"
#include "Components/LightManager.h"
#include "Components/RenderableManager.h"
#include "Components/TransformManager.h"

#include "Types/Skybox.h"

#include "utility/Logger.h"

#include "Core/Camera.h"
#include "Core/Omega_Global.h"
#include "Core/Scene.h"

namespace OmegaEngine
{

OEWorld::OEWorld(OEEngine& engine, VulkanAPI::VkDriver& driver)
    : engine(engine)
    , driver(driver)
{
}


OEWorld::~OEWorld()
{
}

void OEWorld::prepare(Util::String& engName)
{
	// this worlds name, used as a reference
	name = engName;

	// an empty world, so not much to do for now!
}

OEScene* OEWorld::createScene()
{
	auto scene = std::make_unique<OEScene>(*this, engine, driver);
	scenes.emplace_back(std::move(scene));
	return scenes.back().get();
}

OECamera* OEWorld::createCamera()
{
	auto cam = std::make_unique<OECamera>();
	cameras.emplace_back(std::move(cam));
	return cameras.back().get();
}

OESkybox* OEWorld::createSkybox()
{
	auto sb = std::make_unique<OESkybox>(driver);
	skyboxes.emplace_back(std::move(sb));
	return skyboxes.back().get();
}

void OEWorld::update(double time, double dt)
{
}

// ======================= front-end ==================================

Scene* World::createScene()
{
	return static_cast<OEWorld*>(this)->createScene();
}

ObjectManager* World::getObjManager()
{
	return static_cast<OEWorld*>(this)->getObjManager();
}

Camera* World::createCamera()
{
	return static_cast<OEWorld*>(this)->createCamera();
}

Skybox* World::createSkybox()
{
	return static_cast<OEWorld*>(this)->createSkybox();
}

}    // namespace OmegaEngine
