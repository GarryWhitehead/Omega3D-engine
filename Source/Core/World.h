#pragma once

#include "Core/ObjectManager.h"

#include "Models/Formats/GltfModel.h"

#include "OEMaths/OEMaths.h"

#include "utility/CString.h"

#include <unordered_map>
#include <vector>

namespace VulkanAPI
{
class VkDriver;
}

namespace OmegaEngine
{
// forward declerartions
class Scene;
class Object;
class Engine;

class World
{
public:
	World(Engine& engine, VulkanAPI::VkDriver& driver);
	~World();

	void update(double time, double dt);

	/**
	 * prepares this world ready for recieving a scene
	 * @param name An identiying name for this world. 
	 */
	void prepare(Util::String name);

	/**
	* @brief creates a new empty scene. This will be passed to the renderer for presentation.
	* @return A pointer to the newly created scene
	*/
	Scene* createScene();

	ObjectManager& getObjManager()
	{
		return objManager;
	}

private:
    
    Engine& engine;
    VulkanAPI::VkDriver& driver;
    
	// name used to identify this world
	Util::String name;

	/// objects assoicated with this scene dealt with by the object manager
	ObjectManager objManager;

	// scenes associated with this world
	std::vector<std::unique_ptr<Scene>> scenes;
};

}    // namespace OmegaEngine