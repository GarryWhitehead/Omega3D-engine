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
	
    // ============ visibility checks and culling ===================
    // first renderables - create a new list, we will use pointers to the renderables
    // contained within the manager though to save needless copies
    
    
    // and prepare the visible lighting list
    
}


}    // namespace OmegaEngine
