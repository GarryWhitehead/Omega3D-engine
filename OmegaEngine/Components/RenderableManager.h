#pragma once

#include "Core/ObjectManager.h"

#include "Types/Object.h"

#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"

#include "Utility/logger.h"
#include "utility/BitsetEnum.h"

#include "Models/MaterialInstance.h"
#include "Models/MeshInstance.h"

#include "Components/ComponentManager.h"

#include <memory>
#include <unordered_map>

#define MESH_INIT_CONTAINER_SIZE 50

namespace OmegaEngine
{
// forard decleartions
class GpuTextureInfo;
class MeshInstance;
class World;
class Object;
class Engine;
class RenderStateBlock;


/**
	* @brief A convienent way to group textures together
	* supports PBR materials - if null, field not used
	*/
struct TextureGroup
{
	TextureGroup() = default;

	~TextureGroup()
	{
		for (size_t i = 0; i < TextureType::Count; ++i)
		{
			if (textures[i])
			{
				delete textures[i];
				textures[i] = nullptr;
			}
		}
	}

	// not copyable
	TextureGroup(const TextureGroup&) = delete;
	TextureGroup& operator=(const TextureGroup&) = delete;

	GpuTextureInfo* textures[TextureType::Count];
};

struct Material
{
	enum class TextureVariants : uint64_t
	{
		HasBaseColour,
		HasNormal,
		HasMetallicRoughness,
		HasOcclusion,
		HasEmissive
	};

	/// The material attributes
	MaterialInstance instance;

	/// shader variants associated with this material
	Util::BitSetEnum<TextureVariants> variantBits;

	/// each material has its own descriptor set
	VulkanAPI::DescriptorSet descrSet;
};

/**
	* @brief A basic struct pointing to the vertex and indices data within
	* the larger buffer. Meshes are made up of sub-meshes or primitives
	* which reference the mesh data. They also contain an index to the material data
	* which is set when a renderable is added
	*/
struct Renderable
{
	/**
         * Specifies a variant to use when compiling the shader
         */
	enum class MeshVariant : uint64_t
	{
		HasSkin,
		TangentInput,
		BiTangentInput,
		HasUv,
		HasNormal,
		HasWeight,
		HasJoint
	};

	/**
	* Bit flags for specifying the visibilty of renderables
	*/
	enum class Visible : uint8_t
	{
		Renderable,
		Shadow
	};

	/// all the model data
	MeshInstance instance;

	/// NOTE: the gltf spec allows primitives to have their own material. This makes life a lot harder and in 99% of cases,
	/// all primitives share the same material. So we only allow one material per mesh - this can be reviewed and changed if the need arises
	int32_t materialId = -1;    ///< set once added to the renderable manager
	Material* material = nullptr;

	/// the render state of this material
	RenderStateBlock* renderState;

	/// variation of the mesh shader
	Util::BitSetEnum<MeshVariant> variantBits;

	// the mesh and material varinats merged - used for looking up the completed shader program
	uint64_t mergedVariant = 0;

	// visibilty of this renderable and their shadow
	Util::BitSetEnum<Visible> visibility;

	/// ============ vulkan backend ========================
	/// This is set by calling **update**

	/// offset into transform buffer for this mesh
	/// A renderable can either be static or skinned - maybe this should be a abstract inherited class
	size_t staticDynamicOffset = 0;
	size_t skinnedDynamicOffset = 0;
};

class RenderableManager : public ComponentManager
{

public:

	RenderableManager(Engine& engine);
	~RenderableManager();

	/// called on a per-frame basis . Updates all textures and ubos on the vullkan backend.
	/// This isn't so expensive as these are persistent resources
	void update();

	/**
	* @brief The main call here - adds a renderable consisting of mesh, and not
	* always, material and texture data. This function adds a number of materials if required
	*/
	void addRenderable(MeshInstance& mesh, MaterialInstance* mat, const size_t matCount, Object& obj);

	// === mesh related functions ===
	/**
	* @brief Adds a mesh and primitives to the manager. 
	* @param mesh The prepared mesh to add
	* @param idx The index derived from calling **getObjIndex**
	* @param offset The material buffer offset which will be added to the 
	* primitive ids. This value is obtained from **addMaterial**
	*/
	void addMesh(Renderable& input, MeshInstance& mesh, const size_t idx, const size_t offset);

	/**
	* @breif Adds a mesh - use this overload when you want to add multiple meshes
	* linked with the same material group. 
	*/
	void addMesh(Renderable& input, MeshInstance& mesh, Object& obj, const size_t offset);

	/**
     * @brief Returns a instance of a mesh based on the specified object
     */
	Renderable& getMesh(const ObjHandle obj);

	// === material related functions ===
	/**
	* @brief Adds a specified number of materials to the manager
	* @param mat A pointer to a group of materials. This must not be a nullptr
	* @param count The number of materials to add
	* @return The starting index in which this group of materials is found at 
	* within the managers larger container
	*/
	size_t addMaterial(Renderable& input, MaterialInstance* mat, const size_t count);

	friend class GBufferFillPass;
	friend class Renderer;

private:
	void updateBuffers();

	bool prepareTexture(Util::String path, GpuTextureInfo* tex);

	bool updateVariants();

private:
	Engine& engine;

	// the buffers containing all the model data
	std::vector<Renderable> renderables;

	// all the materials
	std::vector<Material> materials;

	// and all the textures
	std::vector<TextureGroup> textures;

	// the vertices and indices mega buffer
	std::vector<MeshInstance::VertexBuffer> vertices;
	std::vector<uint32_t> indices;

	// vertex and index buffers in the vulkan backend - all vertices uploaded in one large blob. Access is via offsets
	VulkanAPI::Buffer* vertexBuffer = nullptr;
	VulkanAPI::Buffer* indexBuffer = nullptr;

	// Dirty flags - state whether anything has been added or altered and needs updating on the backend
	bool meshDirty = true;
	bool materialDirty = true;
};

}    // namespace OmegaEngine
