/* Copyright (c) 2018-2020 Garry Whitehead
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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
class OEIndirectLighting;

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
    
    OEIndirectLighting* createIndirectLighting();
    
    // ========= object management ============
    // object creation functions
    OEObject* createObject();

    // Part of the user interface - creates and objects and adds as a parent to the model graph
    OEObject* createParentObj(const OEMaths::mat4f& world);
    
    OEObject* createParentObj(const OEMaths::vec3f& trans, const OEMaths::vec3f& scale, const OEMaths::quatf& rot);

    // Part of the user interface - creates and objects and adds as a child to the specified parent
    // in the model graph. Returns nullptr if parent is not found
    OEObject* createChildObj(OEObject* parent);

    void destroyObject(OEObject* obj);

    std::vector<OEObject*>& getObjectsList();

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
    std::vector<std::unique_ptr<OEIndirectLighting>> ibls;

    // scenes associated with this world
    std::vector<std::unique_ptr<OEScene>> scenes;

    // ============== object management ==================
    uint32_t nextId = 0;

    // the complete list of all objects associated with all registered scenes
    std::vector<OEObject*> objects;

    // ids of objects which has been destroyed and can be re-used
    std::deque<uint32_t> freeIds;
};

} // namespace OmegaEngine
