#include "Scene.h"

#include "Types/Object.h"

#include "Core/World.h"

#include "Components/RenderableManager.h"

namespace OmegaEngine
{

Scene::Scene(World& world)
    : world(world)
{
}

Scene::~Scene()
{
}

void Scene::update()
{
	
}

}    // namespace OmegaEngine
