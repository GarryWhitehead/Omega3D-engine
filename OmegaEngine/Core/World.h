#pragma once

#include "Managers/AnimationManager.h"
#include "Managers/CameraManager.h"
#include "Managers/LightManager.h"
#include "Managers/MeshManager.h"
#include "Managers/ObjectManager.h"
#include "Managers/TransformManager.h"

#include "Resource/ResourceManager.h"

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
class BVH;

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

	// pointers to each manager
	AnimationManager& getAnimManager();
	CameraManager& getCameraManager();
	LightManager& getLightManager();
	MeshManager& getMeshManager();
	TransformManager& getTransManager();
	ResourceManager& getResourceManager();

private:
	// name used to identify this world
	Util::String name;

	// and all the managers that are required to deal with each of the component types
	AnimationManager animManager;
	CameraManager cameraManager;
	LightManager lightManager;
	MeshManager meshManager;
	TransformManager transManager;

	// all resources that are not associated with a manager are dealt with here
	ResourceManager resourceManager;

	// scenes associated with this world
	std::vector<Scene> scenes;
};

}    // namespace OmegaEngine
