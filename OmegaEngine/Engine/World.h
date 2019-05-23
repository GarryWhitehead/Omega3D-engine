#pragma once

#include "OEMaths/OEMaths.h"

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

	enum class Managers
	{
		OE_MANAGERS_MESH	  	 = 1 << 0,
		OE_MANAGERS_LIGHT		 = 1 << 1,
		OE_MANAGERS_TRANSFORM	 = 1 << 2,
		OE_MANAGERS_CAMERA		 = 1 << 3,
		OE_MANAGERS_PHYSICS		 = 1 << 4,
		OE_MANAGERS_COLLISION	 = 1 << 5,
		OE_MANAGERS_MATERIAL	 = 1 << 6,
		OE_MANAGERS_TEXTURE		 = 1 << 7,
		OE_MANAGERS_ALL			 = 1 << 8
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

		struct NodeInfo
		{
			const char* name;
			uint32_t index;

			// rather than using pointers and lots of buffers, to improve cacahe performance, nodes are all stored in one large buffer and referenced via indicies into the buffer
			int32_t parentIndex = -1;
			std::vector<uint32_t> children;

			int32_t meshIndex = -1;

			// transforms associated with this node
			OEMaths::mat4f matrix;
			OEMaths::vec3f translation;
			OEMaths::vec3f scale{ 1.0f };
			OEMaths::mat4f rotation;

			// skinning info
			int32_t skinIndex = -1;

		};

		World();
		World(Managers managers, std::unique_ptr<VulkanAPI::Device>& device, EngineConfig& engineConfig);
		~World();

		bool create(const char* filename);
		void update(double time, double dt);
		void render(double interpolation);

		// gltf based stuff. Will probably be moved into its own sperate file at some point
		void addGltfData(std::string filename, OEMaths::mat4f worldMatix);
		void loadGltfNode(tinygltf::Model& model, tinygltf::Node& node,
			std::unordered_map<uint32_t, Object>& linearisedObjects,
			OEMaths::mat4f worldTransform,
			std::unique_ptr<ObjectManager>& objManager,
			Object* obj, bool childObject,
			uint32_t nodeIndex,
			uint32_t& global_vertexCount, uint32_t& global_indexCount);

	private:

		// managers that deal with entity / object component system
		std::unique_ptr<ObjectManager> objectManager;
		std::unique_ptr<ComponentInterface> componentInterface;
		std::unique_ptr<AnimationManager> animation_manager;

		// all assets that are not associated with a manager are dealt with here
		std::unique_ptr<AssetManager> assetManager;

		// the main rendering system - used for sorting and drawing all renderable objects. TODO: Keeping with the general scheme, this should probably be a manager
		std::unique_ptr<RenderInterface> renderInterface;

		// the octree as a 3d spatial representation of the world
		std::unique_ptr<BVH> bvh;

		bool hasUpdatedOnce = false;
	};

}
