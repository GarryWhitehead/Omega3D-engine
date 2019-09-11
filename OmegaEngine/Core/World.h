#pragma once

#include "Managers/AnimationManager.h"
#include "Managers/AssetManager.h"
#include "Managers/CameraManager.h"
#include "Managers/LightManager.h"
#include "Managers/MaterialManager.h"
#include "Managers/MeshManager.h"
#include "Managers/ObjectManager.h"
#include "Managers/TransformManager.h"

#include "Models/Gltf/GltfModel.h"

#include "OEMaths/OEMaths.h"

#include <memory>
#include <string>
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
	MaterialManager& getMatManager();
	MeshManager& getMeshManager();
	TransformManager& getTransManager();
	AssetManager& getAssetManager();

private:
	// name used to identify this world
	std::string name;

	// and all the managers that are required to deal with each of the component types
	AnimationManager animManager;
	CameraManager cameraManager;
	LightManager lightManager;
	MaterialManager matManager;
	MeshManager meshManager;
	TransformManager transManager;

	// all assets that are not associated with a manager are dealt with here
	AssetManager assetManager;

	// the octree as a 3d spatial representation of the world
	std::unique_ptr<BVH> bvh;

	// scenes associated with this world
	std::vector<Scene> scenes;
};

}    // namespace OmegaEngine