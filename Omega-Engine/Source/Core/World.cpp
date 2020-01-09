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

OEWorld::OEWorld(OEEngine& engine, VulkanAPI::VkDriver& driver) :
    engine(engine),
    driver(driver)
{
}


OEWorld::~OEWorld()
{
}

void OEWorld::prepare(Util::String engName)
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

void OEWorld::update(double time, double dt)
{
	
}

}    // namespace OmegaEngine
