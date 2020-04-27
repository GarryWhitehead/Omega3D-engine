/* Copyright (c) 2018-2020 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "VulkanAPI/Common.h"
#include "VulkanAPI/Shader.h"
#include "utility/BitsetEnum.h"
#include "utility/CString.h"
#include "utility/Compiler.h"
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
            bool useStencil = false;
            vk::StencilOp failOp;
            vk::StencilOp passOp;
            vk::StencilOp depthFailOp;
            vk::CompareOp compareOp;
            uint32_t compareMask = 0;
            uint32_t writeMask = 0;
            uint32_t reference = 0;
            bool frontEqualBack = true;
        };

        /// depth state
        bool testEnable;
        bool writeEnable;
        vk::CompareOp compareOp;

        /// stencil state
        bool stencilTestEnable = VK_FALSE;

        /// only processed if the above is true
        StencilState frontStencil;
        StencilState backStencil;
    };

    struct RasterState
    {
        vk::CullModeFlagBits cullMode = vk::CullModeFlagBits::eNone;
        vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
        vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
        vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
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

    ShaderProgram(VkDriver& driver, uint32_t shaderId);

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

    ShaderBinding::SamplerBinding&
    findSamplerBinding(const Util::String& name, const Shader::Type type);

    ShaderBinding& findShaderBinding(const Shader::Type type);

    std::vector<vk::PipelineShaderStageCreateInfo> getShaderCreateInfo();

    PipelineLayout* getPLineLayout();

    uint8_t getSetCount() const;

    uint32_t getShaderId() const;

    std::vector<ShaderBinding::SamplerBinding>& getMaterialBindings();

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

    // id for this shader is created by hashing the shader file path
    uint32_t shaderId;

    std::vector<ShaderBinding> stages;

    // the total number of sets associated with this shader
    uint8_t setCount = 0;

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
#pragma pack(push, 1)
    struct OE_PACKED ShaderKey
    {
        uint32_t shaderId;
        uint64_t variantBits;
        uint32_t topology;
    };

    // the cached shader key also is keyed by the shader stage
    struct OE_PACKED CachedKey
    {
        uint32_t shaderId;
        uint32_t shaderStage;
        uint64_t variantBits;
        uint32_t topology;
    };
#pragma pack(pop)

    static_assert(
        std::is_pod<ShaderKey>::value, "ShaderKey must be a POD for the hashing to work correctly");
    static_assert(
        std::is_pod<CachedKey>::value, "CachedKey must be a POD for the hashing to work correctly");

    ProgramManager(VkDriver& driver);
    ~ProgramManager();

    /**
     * @brief Creates a shader program from the specified shader key list. The shader stage varaints
     * must have been created by calling **createCachedInstance** before this fucntion is called.
     * Also, the shader must be in a valid state - namely, there must be a vertex shader.
     */
    ShaderProgram* build(ShaderParser& parser, const std::vector<CachedKey>& hashes);

    bool compile(ShaderParser& parser, ShaderProgram* prog);

    /**
     * @brief Creates a new shader program instance. This will be inserted into the map.
     * @param key The shader key which will be used to locate the new instance
     * @return A pointer to the newly created shader program
     */
    ShaderProgram* createNewInstance(const ShaderKey& key);

    /**
     * @brief Checks whether a shader has been created based on the hash
     * @param key The key of the variant to check
     * @return A boolean set to true if the shader exsists, otherwise false
     */
    bool hasShaderVariant(const ShaderKey& key);

    /**
     * @brief Checks whether a shader has been created based on the hash and returns the program if
     * so.
     * @param key The key of the variant to check
     * @return A pointer to a shader program if one exsists with the designated hash. Otherwise
     * returns nullptr
     */
    ShaderProgram* findVariant(const ShaderKey& key);

    ShaderProgram* getVariantOrCreate(
        const Util::String& filename, uint64_t variantBits, uint32_t topology = UINT32_MAX);

    /**
     * @brief Creates a shader fragment that will be cached until ready for use
     */
    ShaderDescriptor* createCachedInstance(const CachedKey& hash, const ShaderDescriptor& descr);

    /**
     * @brief Checks whether a shader fragment has been cached as specified by the hash
     */
    bool hasShaderVariantCached(const CachedKey& hash);

    ShaderDescriptor* findCachedVariant(const CachedKey& hash);

    friend class CBufferManager;

private:
    // =============== shader hasher ======================

    using ShaderHasher = Util::Murmur3Hasher<ShaderKey>;

    struct ShaderEqual
    {
        bool operator()(const ShaderKey& lhs, const ShaderKey& rhs) const
        {
            return lhs.shaderId == rhs.shaderId && lhs.variantBits == rhs.variantBits &&
                lhs.topology == rhs.topology;
        }
    };

    using CachedHasher = Util::Murmur3Hasher<CachedKey>;

    struct CachedEqual
    {
        bool operator()(const CachedKey& lhs, const CachedKey& rhs) const
        {
            return lhs.shaderId == rhs.shaderId && lhs.shaderStage == rhs.shaderStage &&
                lhs.variantBits == rhs.variantBits && lhs.topology == rhs.topology;
        }
    };

private:
    VkDriver& driver;

    // fully compiled, complete shader programs
    std::unordered_map<ShaderKey, ShaderProgram*, ShaderHasher, ShaderEqual> programs;

    // this is where individual shaders are cached until required to assemble into a shader program
    std::unordered_map<CachedKey, ShaderDescriptor, CachedHasher, CachedEqual> cached;
};

} // namespace VulkanAPI
