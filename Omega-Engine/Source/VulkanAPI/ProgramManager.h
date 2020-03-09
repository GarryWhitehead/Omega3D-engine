#pragma once

#include "VulkanAPI/Common.h"
#include "VulkanAPI/Shader.h"
#include "utility/BitsetEnum.h"
#include "utility/CString.h"
#include "utility/MurmurHash.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace VulkanAPI
{

// forward declerations
class VkDriver;
class CmdBuffer;
class ShaderDescriptor;
class ShaderParser;
class Sampler;
class ImageView;
class DescriptorLayout;
class DescriptorPool;
class DescriptorSet;
class PipelineLayout;
class Pipeline;
class Buffer;
class Texture;

/**
 * @brief Specifies the render pipeline state of the shader program
 * Can also be passed explicitly to override pre-exsisting data
 */
struct RenderStateBlock
{
    struct DsState
    {
        struct StencilState
        {
            vk::StencilOp failOp;
            vk::StencilOp passOp;
            vk::StencilOp depthFailOp;
            vk::CompareOp compareOp;
            uint32_t compareMask = 0;
            uint32_t writeMask = 0;
            uint32_t reference = 0;
        };

        /// depth state
        bool testEnable;
        bool writeEnable;
        vk::CompareOp compareOp;

        /// stencil state
        bool stencilTestEnable = VK_FALSE;

        /// only processed if the above is true
        StencilState front;
        StencilState back;
    };

    struct RasterState
    {
        vk::CullModeFlagBits cullMode;
        vk::PolygonMode polygonMode;
        vk::FrontFace frontFace;
        vk::PrimitiveTopology topology;
        bool primRestart = false;
    };

    struct Sampler
    {
        vk::Filter magFilter;
        vk::Filter minFilter;
        vk::SamplerAddressMode addrModeU;
        vk::SamplerAddressMode addrModeV;
        vk::SamplerAddressMode addrModeW;
    };

    DsState dsState;
    RasterState rastState;
    Sampler sampler;
};

struct MaterialBindingInfo
{
    vk::DescriptorSetLayout layout;
    uint8_t set;
};

/**
 * @brief The compiled shader, in a format ready for binding to the pipeline
 */
class ShaderBinding
{
public:
    enum class BufferFlags : uint32_t
    {
        Dynamic,
        None,
        __SENTINEL__
    };

    /**
     * @brief The uniform buffer bindings for the shader stage - uniform, storage and input buffers
     */
    struct BufferBinding
    {
        std::string name;
        uint16_t bind = 0;
        uint16_t set = 0;
        uint32_t size = 0;
        vk::DescriptorType type;
        Util::BitSetEnum<BufferFlags> flags;
    };

    /**
     * @brief The sampler bindings for the shder stage. These can be sampler2D, sampler3D and
     * samplerCube
     */
    struct SamplerBinding
    {
        std::string name;
        uint16_t bind = 0;
        uint16_t set = 0;
        vk::DescriptorType type;
    };

    struct SpecConstantBinding
    {
        std::string name;
        uint16_t id = 0;
    };

    /**
     * @brief States the ouput from the fragment shader - into a buffer specified by the render pass
     */
    struct RenderTarget
    {
        uint16_t location = 0;
        vk::Format format;
    };

public:
    ShaderBinding(VkContext& context, const Shader::Type type)
        : shader(std::make_unique<Shader>(context, type))
    {
    }

    std::unique_ptr<Shader>& getShader()
    {
        return shader;
    }

    friend class ShaderProgram;
    friend class ShaderCompiler;

private:
    std::vector<BufferBinding> bufferBindings;
    std::vector<SamplerBinding> samplerBindings;
    std::vector<RenderTarget> renderTargets;

    // Specialisation constant are finalised at the pipeline creation stage
    std::vector<SpecConstantBinding> constants;

    // the compiled shader in a format ready for binding to the pipeline
    std::unique_ptr<Shader> shader;
};

/**
 * @brief All the data obtained when compiling the glsl shader json file
 */
class ShaderProgram
{
public:
    /**
     * @brief Input semenatics of the shader stage. Used by the pipeline attributes
     * Only used required for the vertex shader hence why it exsists outside of the shaderbinding
     * struct
     */
    struct InputBinding
    {
        uint16_t loc = 0;
        uint32_t stride = 0;
        vk::Format format;
    };

    struct ConstantInfo
    {
        enum Type
        {
            Float,
            Int
        };

        Util::String name;
        Util::String value;
        Type type;
        Shader::Type stage;
    };

    ShaderProgram(VkDriver& driver);

    /**
     * @brief Compiles the parsed json file into data that can be used by the vulkan backend
     * Note: You must have parsed the json file by calling **parse** before calling this function
     * otherwise it will return an error
     */
    bool prepare(ShaderParser& parser);

    /**
     * @brief Adds a shader variant for a specifed stage to the list
     */
    void addVariant(const Util::String& definition, uint8_t value, Shader::Type stage);

    void addVariants(GlslCompiler::VariantMap& map, const Shader::Type type);

    /**
     * @brief Sorts variants by specified stage, in a format ready for passing to the shader
     * compiler
     */
    std::vector<VulkanAPI::Shader::VariantInfo> sortVariants(Shader::Type stage);

    void overrideRenderState(RenderStateBlock* renderState);

    ShaderBinding::SamplerBinding& findSamplerBinding(Util::String name, const Shader::Type type);

    ShaderBinding& findShaderBinding(const Shader::Type type);

    std::vector<vk::PipelineShaderStageCreateInfo> getShaderCreateInfo();

    PipelineLayout* getPLineLayout();
    
    MaterialBindingInfo* getMaterialBindingInfo();
    
    /**
     * @brief Updates a spec constant which must have been stated in the shader json with a new
     * value which will set at pipeline generation time. If the spec constant isn't uodated, then
     * the const value stated in the json will be used Note: only integers and floats supported at
     * present
     */
    template <typename T>
    void updateConstant(Util::String name, T value, Shader::Type stage)
    {
        Util::String strVal = Util::String::valueToString(value);
        if constexpr (std::is_same_v<T, int>)
        {
            constants.emplace_back(ConstantInfo {name, strVal, ConstantInfo::Int, stage});
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            constants.emplace_back(ConstantInfo {name, strVal, ConstantInfo::Float, stage});
        }
    }

    friend class ShaderCompiler;
    friend class CmdBuffer;
    friend class Pipeline;

private:
    VkDriver& driver;

    std::vector<ShaderBinding> stages;

    // this block overrides all render state for this shader.
    std::unique_ptr<RenderStateBlock> renderState;

    // input bindings for the vertex shader
    std::vector<InputBinding> inputs;
    
    // the material bindings are stored here as we only expect this to be registered from one shader
    std::vector<ShaderBinding::SamplerBinding> materialBindings;
    
    std::unique_ptr<PipelineLayout> pLineLayout;

    // shader constants to add during compiler time
    std::vector<ConstantInfo> constants;

    // shader variants which are added at compile time
    std::vector<VulkanAPI::Shader::VariantInfo> variants;
};


class ProgramManager
{
public:
    struct ShaderKey
    {
        const char* name = nullptr;
        uint64_t variantBits = 0;
        // this equates to vk::PrimitiveTopology but use a int to allow for a "not-used" flag - aka
        // UINT32 MAX
        uint32_t topology = UINT32_MAX;
    };

    ProgramManager(VkDriver& driver);
    ~ProgramManager();

    /**
     * @brief Creates a shader program from the specified shader key list. The shader stage varaints
     * must have been created by calling **createCachedInstance** before this fucntion is called.
     * Also, the shader must be in a valid state - namely, there must be a vertex shader.
     */
    ShaderProgram* build(ShaderParser& parser, std::vector<ShaderKey>& hashes);

    bool compile(ShaderParser& parser, ShaderProgram* prog);

    /**
     * @brief Creates a new shader program instance. This will be inserted into the map.
     * @param name The name of the shader to find - the filename
     * @param renderBlock Whether this shader has a render override block
     * @param variantBits The variant flags used by this shader
     * @return A pointer to the newly created shader program
     */
    ShaderProgram* createNewInstance(ShaderKey& hash);

    /**
     * @brief Checks whether a shader has been created based on the hash
     * @param hash The key of the variant to check
     * @return A boolean set to true if the shader exsists, otherwise false
     */
    bool hasShaderVariant(ShaderKey& hash);

    /**
     * @brief Checks whether a shader has been created based on the hash and returns the program if
     * so.
     * @param hash The key of the variant to check
     * @return A pointer to a shader program if one exsists with the designated hash. Otherwise
     * returns nullptr
     */
    ShaderProgram* findVariant(ShaderKey& hash);

    ShaderProgram* getVariant(ProgramManager::ShaderKey& key);

    /**
     * @brief Creates a shader fragment that will be cached until ready for use
     */
    ShaderDescriptor*
    createCachedInstance(ShaderKey& hash, ShaderDescriptor& descr);

    /**
     * @brief Checks whether a shader fragment has been cached as specified by the hash
     */
    bool hasShaderVariantCached(ShaderKey& hash);

    ShaderDescriptor* findCachedVariant(ShaderKey& hash);

    friend class CBufferManager;

private:
    // =============== shader hasher ======================

    using ShaderHasher = Util::Murmur3Hasher<ShaderKey>;

    struct ShaderEqual
    {
        bool operator()(const ShaderKey& lhs, const ShaderKey& rhs) const
        {
            return lhs.name == rhs.name && lhs.variantBits == rhs.variantBits &&
                lhs.topology == rhs.topology;
        }
    };

private:
    VkDriver& driver;

    // fully compiled, complete shader programs
    std::unordered_map<ShaderKey, ShaderProgram*, ShaderHasher, ShaderEqual> programs;

    // this is where individual shaders are cached until required to assemble into a shader program
    std::unordered_map<ShaderKey, ShaderDescriptor, ShaderHasher, ShaderEqual> cached;
};

} // namespace VulkanAPI
