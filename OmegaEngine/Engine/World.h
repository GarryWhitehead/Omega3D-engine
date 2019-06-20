#pragma once

#include "OEMaths/OEMaths.h"
#include "Models/GltfModel.h"
#include "Managers/CameraManager.h"

#include "tiny_gltf.h"

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

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
	enum class LightType;

	enum class Managers
	{
		OE_MANAGERS_MESH	  	 = 1 << 0,
		OE_MANAGERS_LIGHT		 = 1 << 1,
		OE_MANAGERS_TRANSFORM	 = 1 << 2,
		OE_MANAGERS_CAMERA		 = 1 << 3,
		OE_MANAGERS_PHYSICS		 = 1 << 4,
		OE_MANAGERS_COLLISION	 = 1 << 5,
		OE_MANAGERS_MATERIAL	 = 1 << 6,
		OE_MANAGERS_ALL			 = 1 << 7
	};

	// bitwise overload so casts aren't needed
	inline bool operator& (Managers a, Managers b)
	{
		return static_cast<int>(a) & static_cast<int>(b);
	}

	inline Managers operator| (Managers a, Managers b)
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

		// middle man between object manager and user side
		Object* createObject();

		// spacial function which creates the node tree and adds the appropiate components from a gltf model
		Object* createGltfModelObject(std::unique_ptr<GltfModel::Model>& model, bool useMaterial);

		// other user friendly middle-man functions that avoid exposing the managers to the user
		void addSkybox(const std::string& filename, float blurFactor);
		void addCameraToWorld(OEMaths::vec3f& startPosition = OEMaths::vec3f{ 0.0f, 0.0f, 0.0f }, 
								float fov = 40.0f, 
								float zNear = 1.0f, 
								float zFar = 1000.0f, 
								float aspect = 0.8f, 
								float velocity = 0.5f, 
								Camera::CameraType type = Camera::CameraType::FirstPerson);

		void addLightToWorld(const LightType type, 
								OEMaths::vec3f position, OEMaths::vec3f target, 
								OEMaths::vec3f colour, 
								float radius, float fov, 
								float innerCone = 0.0f, float outerCone = 0.0f);

		void update(double time, double dt);
		void render(double interpolation);

	private:

		void createGltfModelObjectRecursive(std::unique_ptr<ModelNode>& node, Object* parentObject, 
			const uint32_t materialOffset, const uint32_t skinOffset, const uint32_t animationOffset);

		std::string name;

		// managers that deal with entity / object component system
		std::unique_ptr<ObjectManager> objectManager;
		std::unique_ptr<ComponentInterface> componentInterface;
		std::unique_ptr<AnimationManager> animationManager;

		// all assets that are not associated with a manager are dealt with here
		std::unique_ptr<AssetManager> assetManager;

		// the main rendering system - used for sorting and drawing all renderable objects. TODO: Keeping with the general scheme, this should probably be a manager
		std::unique_ptr<RenderInterface> renderInterface;

		// the octree as a 3d spatial representation of the world
		std::unique_ptr<BVH> bvh;

		bool hasUpdatedOnce = false;
	};

}
