#pragma once

#include "omega-engine/World.h"

#include "Core/ObjectManager.h"

#include "ModelImporter/Formats/GltfModel.h"

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
class OEScene;
class Object;
class OEEngine;

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
	void prepare(Util::String name);

	/**
	* @brief creates a new empty scene. This will be passed to the renderer for presentation.
	* @return A pointer to the newly created scene
	*/
	OEScene* createScene();

	OEObjectManager& getObjManager()
	{
		return objManager;
	}

private:
    
    OEEngine& engine;
    VulkanAPI::VkDriver& driver;
    
	// name used to identify this world
	Util::String name;

	/// objects assoicated with this scene dealt with by the object manager
	OEObjectManager objManager;

	// scenes associated with this world
	std::vector<std::unique_ptr<OEScene>> scenes;
};

}    // namespace OmegaEngine
