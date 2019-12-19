#include "World.h"

#include "Components/AnimationManager.h"
#include "Components/LightManager.h"
#include "Components/RenderableManager.h"
#include "Components/TransformManager.h"

#include "utility/Logger.h"

#include "Core/Omega_Global.h"
#include "Core/Scene.h"

namespace OmegaEngine
{
World::World(Engine& engine, VulkanAPI::VkDriver& driver) :
    engine(engine),
    driver(driver)
{
}


World::~World()
{
}

void World::prepare(Util::String engName)
{
	// this worlds name, used as a reference
	name = engName;

	// an empty world, so not much to do for now!
}

Scene* World::createScene()
{
	auto scene = std::make_unique<Scene>(*this, engine, driver);
	scenes.emplace_back(std::move(scene));
	return scenes.back().get();
}

void World::update(double time, double dt)
{
	
}

}    // namespace OmegaEngine
