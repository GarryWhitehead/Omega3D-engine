#pragma once

#include "Components/ComponentManager.h"
#include "ImageUtils/MappedTexture.h"
#include "ModelImporter/MeshInstance.h"
#include "ModelImporter/MaterialInstance.h"
#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"
#include "VulkanAPI/Shader.h"
#include "omega-engine/RenderableManager.h"
#include "utility/BitsetEnum.h"
#include "utility/CString.h"
#include "utility/Logger.h"

#include <array>
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
} // namespace VulkanAPI

namespace OmegaEngine
{
// forard decleartions
class OEWorld;
class OEObject;
class OEEngine;
class RenderStateBlock;
class MappedTexture;
class SkinInstance;
class NodeInstance;


/**
 * @brief A convienent way to group textures together
 * supports PBR materials - if null, field not used
 */
struct TextureGroup
{
    TextureGroup() = default;
    ~TextureGroup() = default;

    // no copy or copy assignment
    TextureGroup(const TextureGroup&) = delete;
    TextureGroup& operator=(const TextureGroup&) = delete;
    // only moveable
    TextureGroup(TextureGroup&&) = default;
    TextureGroup& operator=(TextureGroup&&) = default;

    static Util::String texTypeToStr(const int type);

    // A texture for each pbr type
    std::array<MappedTexture*, MaterialInstance::TextureType::Count> textures {nullptr};

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
    ~Material() = default;

    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;
    Material(Material&&) = default;
    Material& operator=(Material&&) = default;

    static VulkanAPI::GlslCompiler::VariantMap
    createVariants(Util::BitSetEnum<Material::Variants>& bits);

    void addVariant(const MaterialInstance::TextureType type);

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

    static VulkanAPI::GlslCompiler::VariantMap
    createVariants(Util::BitSetEnum<MeshInstance::Variant>& bits);

    /// all the model data
    MeshInstance* instance;

    /// NOTE: the gltf spec allows primitives to have their own material. This makes life a lot
    /// harder and in 99% of cases, all primitives share the same material. So we only allow one
    /// material per mesh - this can be reviewed and changed if the need arises

    /// set once added to the renderable manager. Used for render queue sorting
    int32_t materialId = -1;
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

class OERenderableManager : public ComponentManager, public RenderableManager
{

public:
    OERenderableManager(OEEngine& engine);
    ~OERenderableManager();

    /// called on a per-frame basis . Updates all textures and ubos on the vullkan backend.
    /// This isn't so expensive as these are persistent resources
    bool update();

    /**
     * @brief The main call here - adds a renderable consisting of mesh, and not
     * always, material and texture data. This function adds a number of materials if required
     */
    void addRenderable(MeshInstance* mesh, MaterialInstance* mat, OEObject& obj);

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
    void addMesh(Renderable& input, MeshInstance* mesh, OEObject& obj, const size_t offset);

    /**
     * @brief Returns a instance of a mesh based on the specified object
     */
    Renderable& getMesh(const ObjectHandle& obj);

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

    MappedTexture* prepareTexture(Util::String& path);

    bool updateVariants();

private:
    OEEngine& engine;

    // the buffers containing all the model data
    std::vector<Renderable> renderables;

    // all the materials
    std::vector<Material> materials;

    // and all the textures
    std::vector<TextureGroup> textures;

    // Dirty flags - state whether anything has been added or altered and needs updating on the
    // backend
    bool meshDirty = true;
    bool materialDirty = true;
};

} // namespace OmegaEngine
