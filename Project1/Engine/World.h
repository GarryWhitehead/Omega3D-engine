#pragma once

#include "Vulkan/Device.h"
#include "OEMaths/OEMaths.h"

#include <string>
#include <vector>
#include <memory>

namespace OmegaEngine
{
	// forward declerartions
	class RenderInterface;
	class ComponentInterface;
	class AnimationManager;
	class ObjectManager;
	struct Object;

	enum class Managers
	{
		OE_MANAGERS_MESH	  	 = 1 << 0,
		OE_MANAGERS_LIGHT		 = 1 << 1,
		OE_MANAGERS_TRANSFORM	 = 1 << 2,
		OE_MANAGERS_CAMERA		 = 1 << 3,
		OE_MANAGERS_PHYSICS		 = 1 << 4,
		OE_MANAGERS_COLLISION	 = 1 << 5,
		OE_MANAGERS_TRANSFORM	 = 1 << 6,
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

		World(Managers managers, VulkanAPI::Device device);
		~World();

		bool create(const char* filename);
		void update(double time, double dt);
		void render(double interpolation);

		// gltf based stuff. Will probably be moved into its own sperate file at some point
		void addGltfData(const char* filename, OEMaths::mat4f world_mat);
		void loadGltfNode(tinygltf::Model& model, tinygltf::Node& node, std::vector<Object>& linearised_objects, OEMaths::mat4f world_transform, std::unique_ptr<ObjectManager>& objManager, Object& obj, bool childObject);

	private:

		// managers that deal with entity / object component system
		std::unique_ptr<ObjectManager> objectManager;
		std::unique_ptr<ComponentInterface> component_interface;
		std::unique_ptr<AnimationManager> animation_manager;

		// the main rendering system - used for sorting and drawing all renderable objects. TODO: Keeping with the general scheme, this should probably be a manager
		std::unique_ptr<RenderInterface> render_interface;
	};

}
