#include "World.h"

#include "Components/AnimationManager.h"
#include "Components/LightManager.h"
#include "Components/RenderableManager.h"
#include "Components/TransformManager.h"
#include "Core/Camera.h"
#include "Core/Scene.h"
#include "Types/Object.h"
#include "Types/Skybox.h"
#include "utility/Logger.h"

namespace OmegaEngine
{

OEWorld::OEWorld(OEEngine& engine, VulkanAPI::VkDriver& driver) : engine(engine), driver(driver)
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

OEObject* OEWorld::createObject()
{
    uint64_t id = 0;
    if (!freeIds.empty() && freeIds.size() > MINIMUM_FREE_IDS)
    {
        id = freeIds.front();
        freeIds.pop_front();
    }
    else
    {
        id = nextId++;
    }

    OEObject* object = new OEObject(id);
    objects.emplace_back(object);
    return objects.back();
}

void OEWorld::destroyObject(OEObject* obj)
{
    size_t count = 0;
    for (auto& object : objects)
    {
        if (*obj == *object)
        {
            break;
        }
        ++count;
    }
    // completley remove from the list - costly!
    objects.erase(objects.begin() + count);
    freeIds.push_front(obj->getId());
}

OEObject* OEWorld::createParentObj(const OEMaths::mat4f& worldMat)
{
    OEObject* obj = createObject();
    if (obj)
    {
        modelGraph.addNode(obj, worldMat);
    }
    return obj;
}

OEObject* OEWorld::createParentObj(
    const OEMaths::vec3f& trans, const OEMaths::vec3f& scale, const OEMaths::quatf& rot)
{
    return createParentObj(OEMaths::mat4f::translate(trans) * rot * OEMaths::mat4f::scale(scale));
}

OEObject* OEWorld::createChildObj(OEObject* parent)
{
    OEObject* obj = createObject();
    if (obj)
    {
        modelGraph.addChildNode(parent, obj);
    }
    return obj;
}

std::vector<OEObject*>& OEWorld::getObjectsList()
{
    return objects;
}

ModelGraph& OEWorld::getModelGraph()
{
    return modelGraph;
}

// ===================== front-end =====================================

Object* World::createObj()
{
    return static_cast<OEWorld*>(this)->createObject();
}

Object* World::createParentObj(const OEMaths::mat4f& world)
{
    return static_cast<OEWorld*>(this)->createParentObj(world);
}

Object* World::createParentObj(
    const OEMaths::vec3f& trans, const OEMaths::vec3f& scale, const OEMaths::quatf& rot)
{
    return static_cast<OEWorld*>(this)->createParentObj(trans, scale, rot);
}

Object* World::createChildObj(Object* obj)
{
    return static_cast<OEWorld*>(this)->createChildObj(static_cast<OEObject*>(obj));
}

void World::destroyObject(Object* obj)
{
    static_cast<OEWorld*>(this)->destroyObject(static_cast<OEObject*>(obj));
}

Scene* World::createScene()
{
    return static_cast<OEWorld*>(this)->createScene();
}

Camera* World::createCamera()
{
    return static_cast<OEWorld*>(this)->createCamera();
}

Skybox* World::createSkybox()
{
    return static_cast<OEWorld*>(this)->createSkybox();
}

} // namespace OmegaEngine
