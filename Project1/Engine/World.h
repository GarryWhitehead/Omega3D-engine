#pragma once

#include <string>
#include <vector>

#include "Vulkan/Device.h"
#include "OEMaths/OEMaths.h"
#include "tiny_gltf.h"

namespace OmegaEngine
{
	// forward declerartions
	class MaterialManager;
	class TextureManager;
	class ComponentInterface;
	class ObjectManager;
	class Camera;
	struct Object;

	enum class Managers
	{
		OE_MANAGERS_MESH	  	 = 1 << 0,
		OE_MANAGERS_LIGHT		 = 1 << 1,
		OE_MANAGERS_TRANSFORM	 = 1 << 2,
		OE_MANAGERS_ANIMATION	 = 1 << 3,
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
		void update();
		void addGltfData(const char* filename, OEMaths::mat4f world_mat);
		void loadGltfNode(tinygltf::Model& model, tinygltf::Node& node, OEMaths::mat4f world_transform, std::unique_ptr<ObjectManager>& objManager, Object& obj, bool childObject);

	private:

		// managers that deal with entity / object component system
		std::unique_ptr<ObjectManager> objectManager;
		std::unique_ptr<ComponentInterface> compSystem;
		std::unique_ptr<RenderInterface> render_interface;

		// other managers 
		std::unique_ptr<MaterialManager> materialManager;
		std::unique_ptr<TextureManager> textureManager;

		// cameras registered with this world
		std::vector<std::unique_ptr<Camera> > cameras;
		uint8_t currentCameraIndex;
	};

}
