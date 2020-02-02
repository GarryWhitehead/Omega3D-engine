#pragma once

#include "Core/ModelGraph.h"
#include "ModelImporter/Formats/GltfModel.h"
#include "OEMaths/OEMaths.h"
#include "omega-engine/World.h"
#include "utility/CString.h"

#include <memory>
#include <unordered_map>
#include <vector>
#include <deque>

#define MINIMUM_FREE_IDS 100

namespace VulkanAPI
{
class VkDriver;
}

namespace OmegaEngine
{
// forward declerartions
class OEScene;
class OEEngine;
class OECamera;
class OESkybox;

class OEWorld : public World
{
public:
    OEWorld(OEEngine& engine, VulkanAPI::VkDriver& driver);
    ~OEWorld();

    void update(double time, double dt);

    /**
     * prepares this world ready for recieving a scene
     * @param name An identiying name for this world.
     */
    void prepare(Util::String& name);

    /**
     * @brief creates a new empty scene. This will be passed to the renderer for presentation.
     * @return A pointer to the newly created scene
     */
    OEScene* createScene();

    OECamera* createCamera();

    OESkybox* createSkybox();

    // ========= object management ============
    // object creation functions
    OEObject* createObject();

    // Part of the user interface - creates and objects and adds as a parent to the model graph
    OEObject* createParentObj();

    // Part of the user interface - creates and objects and adds as a child to the specified parent
    // in the model graph. Returns nullptr if parent is not found
    OEObject* createChildObj(OEObject* parent);

    void destroyObject(OEObject* obj);

    std::vector<OEObject>& getObjectsList();

    ModelGraph& getModelGraph();

private:
    OEEngine& engine;
    VulkanAPI::VkDriver& driver;

    // name used to identify this world
    Util::String name;

    // the model graph for renderable objects - links parent objects with their children.
    // will probably become more of a scene graph at some point.
    // linked with the object manager - filled at the point of a call to createParentObj() or
    // createChildObj()
    ModelGraph modelGraph;

    std::vector<std::unique_ptr<OECamera>> cameras;
    std::vector<std::unique_ptr<OESkybox>> skyboxes;

    // scenes associated with this world
    std::vector<std::unique_ptr<OEScene>> scenes;

    // ============== object management ==================
    uint32_t nextId = 0;

    // this is an unordered map so we can quickly find objects based on their id. Saves having to
    // iterate through a vector which could be costly time-wise
    std::vector<OEObject> objects;

    // ids of objects which has been destroyed and can be re-used
    std::deque<uint32_t> freeIds;
};

} // namespace OmegaEngine
