#pragma once

#include "Core/ObjectManager.h"

#include "Models/Formats/GltfModel.h"

#include "OEMaths/OEMaths.h"

#include "utility/String.h"

#include <unordered_map>
#include <vector>

namespace VulkanAPI
{
class Device;
}

namespace OmegaEngine
{
// forward declerartions
class Scene;
class Object;

class World
{
public:
	World();
	~World();

	void update(double time, double dt);

	/**
	 * prepares this world ready for recieving a scene
	 * @param name An identiying name for this world. 
	 */
	void prepare(const std::string& name);

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
	// name used to identify this world
	Util::String name;

	/// objects assoicated with this scene dealt with by the object manager
	ObjectManager objManager;

	// scenes associated with this world
	std::vector<Scene> scenes;
};

}    // namespace OmegaEngine
