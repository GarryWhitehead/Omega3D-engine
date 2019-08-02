#pragma once

#include "Managers/CameraManager.h"
#include "Managers/LightManager.h"
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
class AnimationManager;
class AssetManager;
class ObjectManager;
class Object;
class BVH;
struct EngineConfig;

enum class Managers
{
	OE_MANAGERS_MESH = 1 << 0,
	OE_MANAGERS_LIGHT = 1 << 1,
	OE_MANAGERS_TRANSFORM = 1 << 2,
	OE_MANAGERS_CAMERA = 1 << 3,
	OE_MANAGERS_PHYSICS = 1 << 4,
	OE_MANAGERS_COLLISION = 1 << 5,
	OE_MANAGERS_MATERIAL = 1 << 6,
	OE_MANAGERS_ANIMATION = 1 << 7,
	OE_MANAGERS_ALL = 1 << 8
};

// bitwise overload so casts aren't needed
inline bool operator&(Managers a, Managers b)
{
	return static_cast<int>(a) & static_cast<int>(b);
}

inline Managers operator|(Managers a, Managers b)
{
	return static_cast<Managers>(static_cast<int>(a) | static_cast<int>(b));
}

class World
{
public:
	World();
	World(Managers managers, std::unique_ptr<VulkanAPI::Device>& device, EngineConfig& engineConfig);
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
	                         float fov, const OEMaths::vec3f& dir, float radius, float scale, float offset,
	                         const LightAnimateType animType = LightAnimateType::Static, float animVel = 0.0f);

	void World::addPointLightToWorld(const OEMaths::vec3f& position, const OEMaths::vec3f& target,
	                                 const OEMaths::vec3f& colour, float fov, float radius,
	                                 const LightAnimateType animType = LightAnimateType::Static, float animVel = 0.0f);

	void update(double time, double dt);
	void render(double interpolation);

private:
	void createGltfModelObjectRecursive(std::unique_ptr<GltfModel::ModelNode>& node, Object* parentObject,
	                                    const uint32_t materialOffset, const uint32_t skinOffset,
	                                    const uint32_t animationOffset);

	std::string name;

	// managers that deal with entity / object component system
	std::unique_ptr<ObjectManager> objectManager;
	std::unique_ptr<ComponentInterface> componentInterface;

	// all assets that are not associated with a manager are dealt with here
	std::unique_ptr<AssetManager> assetManager;

	// the main rendering system - used for sorting and drawing all renderable objects. TODO: Keeping with the general scheme, this should probably be a manager
	std::unique_ptr<RenderInterface> renderInterface;

	// the octree as a 3d spatial representation of the world
	std::unique_ptr<BVH> bvh;

	bool hasUpdatedOnce = false;
};

}    // namespace OmegaEngine
