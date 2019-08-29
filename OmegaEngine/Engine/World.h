#pragma once

#include "Managers/AnimationManager.h"
#include "Managers/CameraManager.h"
#include "Managers/LightManager.h"
#include "Managers/MaterialManager.h"
#include "Managers/MeshManager.h"
#include "Managers/TransformManager.h"

#include "ObjectInterface/ObjectManager.h"

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
class RenderInterface;
class ComponentInterface;
class AssetManager;
class ObjectManager;
class Object;
class BVH;
struct EngineConfig;

class World
{
public:
	World();
	World(std::unique_ptr<VulkanAPI::Device>& device, EngineConfig& engineConfig);
	~World();

	// user interface stuff - world creation
	bool create(const std::string& filename, const std::string& name);
	void create(const std::string& name);

	// middle man between object manager and user side - adds world transform component
	Object* createObject(const OEMaths::vec3f& position, const OEMaths::vec3f& scale, const OEMaths::quatf& rotation);

	void extractGltfModelAssets(std::unique_ptr<GltfModel::Model>& model, uint32_t& materialOffset,
	                            uint32_t& skinOffset, uint32_t& animationOffset);

	// spacial function which creates the node tree and adds the appropiate components from a gltf model
	Object* createGltfModelObject(std::unique_ptr<GltfModel::Model>& model, const OEMaths::vec3f& position,
	                              const OEMaths::vec3f& scale, const OEMaths::quatf& rotation, bool useMaterial);

	// other user friendly middle-man functions that avoid exposing the managers to the user
	void addSkybox(const std::string& filename, float blurFactor);
	void addCameraToWorld(OEMaths::vec3f& startPosition = OEMaths::vec3f{ 0.0f, 0.0f, 6.0f }, float fov = 40.0f,
	                      float zNear = 1.0f, float zFar = 1000.0f, float aspect = 1.7f, float velocity = 0.5f,
	                      Camera::CameraType type = Camera::CameraType::FirstPerson);


	void addSpotLightToWorld(const OEMaths::vec3f& position, const OEMaths::vec3f& target, const OEMaths::vec3f& colour,
	                         float fov, float intensity, float fallOut, float innerCone, float outerCone,
	                         const LightAnimateType animType = LightAnimateType::Static, float animVel = 0.0f);

	void addPointLightToWorld(const OEMaths::vec3f& position, const OEMaths::vec3f& target,
	                          const OEMaths::vec3f& colour, float fov, float intensity, float fallOut,
	                          const LightAnimateType animType = LightAnimateType::Static, float animVel = 0.0f);

	void addDirectionalLightToWorld(const OEMaths::vec3f& position, const OEMaths::vec3f& target,
	                                const OEMaths::vec3f& colour, float fov, float intensity);

	void update(double time, double dt);
	void render(double interpolation);

private:
	void createGltfModelObjectRecursive(std::unique_ptr<GltfModel::ModelNode>& node, Object* parentObject,
	                                    const uint32_t materialOffset, const uint32_t skinOffset,
	                                    const uint32_t animationOffset);

	std::string name;

	/// contains all the objects associated with this world
	ObjectManager objectManager;
	
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

	bool hasUpdatedOnce = false;
};

}    // namespace OmegaEngine
