#pragma once

#include "VulkanAPI/Common.h"
#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/VkDriver.h"

#include "utility/BitSetEnum.h"
#include "utility/String.h"

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

/**
* @brief Specifies the render pipeline state of the shader program
* Can also be passed explicitly to override pre-exsisting data
*/
struct RenderStateBlock
{
	struct RasterState
	{
		vk::CullModeFlagBits cullMode = vk::CullModeFlagBits::eNone;
		vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
		vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
	};

	struct Sampler
	{
		vk::Filter magFilter;
		vk::Filter minFilter;
		vk::SamplerAddressMode addrModeU;
		vk::SamplerAddressMode addrModeV;
		vk::SamplerAddressMode addrModeW;
	};

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
     * The uniform buffer bindings for the shader stage - uniform, storage and input buffers
     */
	struct BufferBinding
	{
		Util::String name;
		int16_t bind = 0;
		uint16_t set = 0;
		uint32_t size = 0;
		Shader::Type shader;
	};

	/**
     * The sampler bindings for the shder stage. These can be sampler2D, sampler3D and samplerCube
     */
	struct SamplerBinding
	{
		Util::String name;
		int16_t bind = 0;
		uint16_t set = 0;
		Shader::Type shader;
	};

	/**
     * Input semenatics of the shader stage. Used by the pipeline attributes
     */
	struct InputBinding
	{
		uint16_t location = 0;
		uint32_t stride = 0;
		vk::Format format;
	};

	struct SpecConstantBinding
	{
		Util::String name;
		uint8_t id = 0;
	};

	/**
     * States the ouput from the fragment shader - into a buffer specified by the render pass
     */
	struct RenderTarget
	{
		uint16_t location = 0;
		vk::Format format;
	};

	ShaderBinding() = default;

	friend class ShaderCompiler;

private:
	std::vector<BufferBinding> bufferBindings;
	std::vector<SamplerBinding> samplerBindings;
	std::vector<RenderTarget> renderTargets;
	std::vector<InputBinding> inputs;

	// Specialisation constant are finalised at the pipeline creation stage
	std::vector<SpecConstantBinding> constants;

	// the compiled shader in a format ready for binding to the pipeline
	Shader shader;
};

/**
 * @brief All the data obtained when compiling the glsl shader json file
 */
class ShaderProgram
{
public:
	
	ShaderProgram() = default;

	/**
	* @brief Compiles the parsed json file into data that can be used by the vulkan backend
	* Note: You must have parsed the json file by calling **parse** before calling this function
	* otherwise it will return an error
	*/
	bool prepare(ShaderParser& parser, VkContext& context);

	/**
     * @brief Adds a shader variant for a specifed stage to the list
     */
	void addVariant(Util::String definition, uint8_t value, Shader::Type stage);

	void overrideRenderState(RenderStateBlock* renderState);

	/**
     * @brief Updates a spec constant which must have been stated in the shader json with a new value which will
     * set at pipeline generation time. If the spec constant isn't uodated, then the const value stated in the json will be used
     * Note: only integers and floats supported at present
     */
	void updateConstant(Util::String name, uint32_t value, Shader::Type stage);
	void updateConstant(Util::String name, int32_t value, Shader::Type stage);
	void updateConstant(Util::String name, float value, Shader::Type stage);

	friend class ShaderCompiler;
	friend class CmdBuffer;

private:
	
	std::array<std::unique_ptr<ShaderBinding>, Shader::Type::Count> stages;

	// this block overrides all render state for this shader.
	std::unique_ptr<RenderStateBlock> renderState;

	// used by the vulkan backend
	DescriptorLayout descrLayout;
	PipelineLayout pLineLayout;
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

	ProgramManager(VkDriver& context);
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

	/**
	* @brief Creates a shader fragment that will be cached until ready for use
	*/
	ShaderDescriptor* createCachedInstance(ShaderHash& hash);

	/**
	* @brief Checks whether a shader fragment has been cached as specified by the hash
	*/
	bool hasShaderVariantCached(ShaderHash& hash);

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
	VkDriver& context;

	// fully compiled, complete shader programs
	std::unordered_map<ShaderHash, ShaderProgram*, ShaderHasher, ShaderEqual> programs;

	// this is where individual shaders are cached until required to assemble into a shader program
	std::unordered_map<ShaderHash, ShaderDescriptor, ShaderHasher, ShaderEqual> cached;
};

}    // namespace VulkanAPI
