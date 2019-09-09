#pragma once

#include "Managers/AnimationManager.h"
#include "Managers/CameraManager.h"
#include "Managers/LightManager.h"
#include "Managers/MaterialManager.h"
#include "Managers/MeshManager.h"
#include "Managers/TransformManager.h"
#include "Managers/ObjectManager.h"

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
class AssetManager;
class Object;
class BVH;

class World
{
public:
	World();
	~World();

	/**
	 * prepares this world ready for recieving a scene
	 * @param name An identiying name for this world. 
	 */
	void prepare(const std::string& name);

	void update(double time, double dt);
	void render(double interpolation);

	/// pointers to each manager
	AnimationManager& getAnimManager();
	CameraManager& getCameraManager();
	LightManager& getLightManager();
	MaterialManager& getMatManager();
	MeshManager& getMeshManager();
	TranformManager& getTransManager();
	AssetManager& getAssetManager();

private:

	/// name used to identify this world
	std::string name;
	
	/// and all the managers that are required to deal with each of the component types
	AnimationManager animManager;
	CameraManager cameraManager;
	LightManager lightManager;
	MaterialManager matManager;
	MeshManager meshManager;
	TransformManager transformManager;

	/// all assets that are not associated with a manager are dealt with here
	AssetManager assetManager;

	// the octree as a 3d spatial representation of the world
	std::unique_ptr<BVH> bvh;
};

}    // namespace OmegaEngine
