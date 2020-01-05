#pragma once

#include "VulkanAPI/Common.h"
#include "VulkanAPI/Shader.h"

#include "utility/BitSetEnum.h"
#include "utility/CString.h"

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

/**
* @brief The compiled shader, in a format ready for binding to the pipeline
*/
class ShaderBinding
{
public:
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
	};

	/**
     * @brief The sampler bindings for the shder stage. These can be sampler2D, sampler3D and samplerCube
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
    
    ShaderBinding(VkContext& context) :
        shader(std::make_unique<Shader>(context))
    {}
    
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
     * Only used required for the vertex shader hence why it exsists outside of the shaderbinding struct
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
    
	ShaderProgram(VkContext& context);

	/**
	* @brief Compiles the parsed json file into data that can be used by the vulkan backend
	* Note: You must have parsed the json file by calling **parse** before calling this function
	* otherwise it will return an error
	*/
	bool prepare(ShaderParser& parser);

	/**
     * @brief Adds a shader variant for a specifed stage to the list
     */
	void addVariant(Util::String definition, uint8_t value, Shader::Type stage);
    
    /**
     * @brief Sorts variants by specified stage, in a format ready for passing to the shader compiler
     */
    std::vector<VulkanAPI::Shader::VariantInfo> sortVariants(Shader::Type stage);
    
	void overrideRenderState(RenderStateBlock* renderState);

	ShaderBinding::SamplerBinding& findSamplerBinding(Util::String name, const Shader::Type type);

	// =============== decriptor set functions ==============================

	bool updateDecrSetBuffer(Util::String id, Buffer& buffer);

	bool updateDecrSetImage(Util::String id, Texture& tex);

	// ================== getters ==========================

	std::vector<vk::PipelineShaderStageCreateInfo> getShaderCreateInfo();

	DescriptorLayout* getDescrLayout(uint8_t set);

	DescriptorSet* getDescrSet();
    
    PipelineLayout* getPLineLayout();
    
    /**
    * @brief Updates a spec constant which must have been stated in the shader json with a new value which will
    * set at pipeline generation time. If the spec constant isn't uodated, then the const value stated in the json will be used
    * Note: only integers and floats supported at present
    */
    template <typename T>
    void updateConstant(Util::String name, T value, Shader::Type stage)
    {
        Util::String strVal = Util::String::valueToString(value);
        if constexpr (std::is_same_v<T, int>)
        {
            constants.emplace_back(ConstantInfo{name, strVal, ConstantInfo::Int, stage});
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            constants.emplace_back(ConstantInfo{name, strVal, ConstantInfo::Float, stage});
        }
    }
    
	friend class ShaderCompiler;
	friend class CmdBuffer;
    friend class Pipeline;
    
private:
    
    VkContext& context;
    
	std::vector<ShaderBinding> stages;

	// this block overrides all render state for this shader.
	std::unique_ptr<RenderStateBlock> renderState;
    
    // input bindings for the vertex shader
    std::vector<InputBinding> inputs;
    
	// used by the vulkan backend
    std::unique_ptr<DescriptorPool> descrPool;

	// it's tricky deciding the best place to store descriptor sets. In OE, they are stored in the shader program as only
	// the shader knows the layout. Call **updateDescrSets()** set update the sets with relevant buffer/texture info. This is
	// usually done via the driver and a **updateUniform()** call.
	std::unique_ptr<DescriptorSet> descrSet;
	std::unique_ptr<PipelineLayout> pLineLayout;
    
    // shader constants to add during compiler time
    std::vector<ConstantInfo> constants;
    
    // shader variants which are added at compile time
    std::vector<VulkanAPI::Shader::VariantInfo> variants;
};


class ProgramManager
{
public:
    
	struct ShaderHash
	{
		const char* name;
		uint64_t variantBits;
		vk::PrimitiveTopology* topology;    //< optional (leave null if not needed)
	};

	ProgramManager(VkDriver& driver);
	~ProgramManager();

	/**
	* @brief Creates a shader program from the specified shader key list. The shader stage varaints must
	* have been created by calling **createCachedInstance** before this fucntion is called. Also, the
	* shader must be in a valid state - namely, there must be a vertex shader.
	*/
	ShaderProgram* build(std::vector<ShaderHash>& hashes);

	/**
     * @brief Creates a new shader program instance. This will be inserted into the map.
	 * @param name The name of the shader to find - the filename
     * @param renderBlock Whether this shader has a render override block
     * @param variantBits The variant flags used by this shader
	 * @return A pointer to the newly created shader program
     */
	ShaderProgram* createNewInstance(ShaderHash& hash);

	/**
     * @brief Checks whether a shader has been created based on the hash
     * @param hash The key of the variant to check
     * @return A boolean set to true if the shader exsists, otherwise false
     */
	bool hasShaderVariant(ShaderHash& hash);

	/**
     * @brief Checks whether a shader has been created based on the hash and returns the program if so. 
     * @param hash The key of the variant to check
     * @return A pointer to a shader program if one exsists with the designated hash. Otherwise returns nullptr
     */
	ShaderProgram* findVariant(ShaderHash& hash);
    
    ShaderProgram* getVariant(ProgramManager::ShaderHash& key);
    
	/**
	* @brief Creates a shader fragment that will be cached until ready for use
	*/
	ShaderDescriptor* createCachedInstance(ShaderHash& hash);

	/**
	* @brief Checks whether a shader fragment has been cached as specified by the hash
	*/
	bool hasShaderVariantCached(ShaderHash& hash);

	ShaderDescriptor* findCachedVariant(ShaderHash& hash);

	/**
     * @brief Pushes a buffer dedscriptor to update this frame. This will not check if the id exsists. Thus, will only fail at the point of update, at which
     * point it will throw an exception if it does not exsist
     */
	void pushBufferDescrUpdate(Util::String id, Buffer& buffer);

	/**
    * @brief Pushes a image dedscriptor to update this frame. This will not check if the id exsists. Thus, will only fail at the point of update, at which
    * point it will throw an exception if it does not exsist
    */
	void pushImageDescrUpdate(Util::String id, Texture& tex);

	/**
     * @brief A special function call for updating the material descriptor sets. The reason for updating them in this fashion is because they require the descriptor]
     * layout of the model shader. Rather than passing around the descriptor layout which could be difficult as it depends on whether the shader has actually been
     * initialised, this methid makes it less error prone and keeps the program data contatned within its class
     */
	void pushMatDescrUdpdate(Util::String id, DescriptorSet* set);

private:
    
	/// For all descriptors that need checking, all shaders that are registered for the id, and updates the descr set with the buffer if found.
	void updateBufferDecsrSets();

	/// Similiar to **updateBufferDescrSets** but this is for images
	void updateImageDecsrSets();

	friend class CmdBufferManager;

private:
	// =============== shader hasher ======================

	struct ShaderHasher
	{
		size_t operator()(ShaderHash const& id) const noexcept
		{
			size_t h1 = std::hash<const char*>{}(id.name);
			size_t h2 = std::hash<uint64_t>{}(id.variantBits);
			size_t h3 = std::hash<vk::PrimitiveTopology*>{}(id.topology);
			return h1 ^ (h2 << 1) ^ (h3 << 1);
		}
	};

	struct ShaderEqual
	{
		bool operator()(const ShaderHash& lhs, const ShaderHash& rhs) const
		{
			return lhs.name == rhs.name && lhs.variantBits == rhs.variantBits && lhs.topology == rhs.topology;
		}
	};

private:
	VkDriver& driver;

	// fully compiled, complete shader programs
	std::unordered_map<ShaderHash, ShaderProgram*, ShaderHasher, ShaderEqual> programs;

	// this is where individual shaders are cached until required to assemble into a shader program
	std::unordered_map<ShaderHash, ShaderDescriptor*, ShaderHasher, ShaderEqual> cached;

	// Queued decriptor requiring updating which is done on a per frame basis
	std::vector<std::pair<const char*, Buffer*>> bufferDescrQueue;
	std::vector<std::pair<const char*, Texture*>> imageDescrQueue;
};

}    // namespace VulkanAPI
