#pragma once

#include "Core/ObjectManager.h"

#include "Types/MappedTexture.h"

#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"

#include "utility/Logger.h"
#include "utility/BitSetEnum.h"
#include "utility/CString.h"

#include "Components/ComponentManager.h"

#include <memory>
#include <unordered_map>

#define MESH_INIT_CONTAINER_SIZE 50

// vulkan forward declerations
namespace VulkanAPI
{
class DescriptorSet;
class ShaderProgram;
class VertexBuffer;
class IndexBuffer;
}

namespace OmegaEngine
{
// forard decleartions
class World;
class Object;
class Engine;
class RenderStateBlock;
class MappedTexture;
class MeshInstance;
class MaterialInstance;
class SkinInstance;
class NodeInstance;

/**
 * @brief The user interface for passing models to the manager.
 */
class RenderableInstance
{
public:
    
    RenderableInstance();
    
    RenderableInstance& addMesh(MeshInstance* instance);
    RenderableInstance& addMaterial(MaterialInstance* instance);
    
    // these are stored in the transform manager
    RenderableInstance& addSkin(SkinInstance* instance);
    RenderableInstance& addNode(NodeInstance* instance);
    
    void create(Engine& engine, Object* obj);
    
    friend class RenderableManager;
    
private:
        
    // this is just a transient store, this class does not own these
    MeshInstance* mesh = nullptr;
    MaterialInstance* mat = nullptr;
    SkinInstance* skin = nullptr;
    NodeInstance* node = nullptr;
};

/**
	* @brief A convienent way to group textures together
	* supports PBR materials - if null, field not used
	*/
struct TextureGroup
{
	enum TextureType : size_t
	{
		BaseColour,
		Emissive,
		MetallicRoughness,
		Normal,
		Occlusion,
		Count
	};

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

	static Util::String texTypeToStr(const int type);
    
    // A texture for each pbr type
	MappedTexture* textures[TextureType::Count];
    
    // keep track of the material id these textures are associated with
    Util::String matName;
};

struct Material
{
    enum Variants : uint64_t
    {
       HasBaseColour,
       HasNormal,
       HasUv,
       HasMetallicRoughness,
       HasOcclusion,
       HasEmissive,
       __SENTINEL__
    };
    
    Material() = default;
    ~Material();
    
	/// The material attributes
    MaterialInstance* instance;
    
    // shader variants associated with this material
    Util::BitSetEnum<Variants> variantBits;
    
	/// each material has its own descriptor set
	VulkanAPI::DescriptorSet* descrSet = nullptr;
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
	* Bit flags for specifying the visibilty of renderables
	*/
	enum class Visible : uint8_t
	{
		Render,
		Shadow,
        __SENTINEL__
	};

	/// all the model data
	MeshInstance* instance;

	/// NOTE: the gltf spec allows primitives to have their own material. This makes life a lot harder and in 99% of cases,
	/// all primitives share the same material. So we only allow one material per mesh - this can be reviewed and changed if the need arises
	int32_t materialId = -1;    ///< set once added to the renderable manager. Used for render queue sorting
	Material* material = nullptr;

	/// the render state of this material
	RenderStateBlock* renderState;

	// the mesh and material varinats merged - used for looking up the completed shader program
	uint64_t mergedVariant = 0;

	// visibilty of this renderable and their shadow
	Util::BitSetEnum<Visible> visibility;

	/// ============ vulkan backend ========================
	VulkanAPI::ShaderProgram* program = nullptr;

	/// this is set by the transform manager but held here for convience reasons when drawing
	uint32_t dynamicOffset = 0;

	// pointers to the vertex and index buffers once uploaded to the gpu
	VulkanAPI::VertexBuffer* vertBuffer = nullptr;
	VulkanAPI::IndexBuffer* indexBuffer = nullptr;
    
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
	void addRenderable(MeshInstance* mesh, MaterialInstance* mat, Object& obj);

	// === mesh related functions ===
	/**
	* @brief Adds a mesh and primitives to the manager. 
	* @param mesh The prepared mesh to add
	* @param idx The index derived from calling **getObjIndex**
	* @param offset The material buffer offset which will be added to the 
	* primitive ids. This value is obtained from **addMaterial**
	*/
	void addMesh(Renderable& input, MeshInstance* mesh, const size_t idx, const size_t offset);

	/**
	* @breif Adds a mesh - use this overload when you want to add multiple meshes
	* linked with the same material group. 
	*/
	void addMesh(Renderable& input, MeshInstance* mesh, Object& obj, const size_t offset);

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
	size_t addMaterial(Renderable& input, MaterialInstance* mat);

	friend class GBufferFillPass;
	friend class Renderer;

private:
	void updateBuffers();

	bool prepareTexture(Util::String path, MappedTexture* tex);

	bool updateVariants();

private:
	Engine& engine;

	// the buffers containing all the model data
	std::vector<Renderable> renderables;

	// all the materials
	std::vector<Material> materials;

	// and all the textures
	std::vector<TextureGroup> textures;

	// Dirty flags - state whether anything has been added or altered and needs updating on the backend
	bool meshDirty = true;
	bool materialDirty = true;
};

}    // namespace OmegaEngine
