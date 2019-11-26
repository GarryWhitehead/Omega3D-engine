#pragma once

#include "VulkanAPI/Common.h"
#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/VkDriver.h"

#include "utility/BitSetEnum.h"
#include "utility/String.h"

#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace VulkanAPI
{

// forward declerations
class VkDriver;

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

class ShaderDescriptor
{
public:
	ShaderDescriptor() = default;

	ShaderDescriptor(Shader::StageType type)
	    : type(type)
	{
	}

	// not copyable
	ShaderDescriptor(const ShaderDescriptor&) = delete;
	ShaderDescriptor& operator=(const ShaderDescriptor&) = delete;

	/// generic descriptor for different shader types
	struct Descriptor
	{
		std::string name;
		std::string type;
		std::string id;                     //< Buffers only - optional sub-name for a struct
		uint16_t groupId;                   //< specifies an explicit set number
		std::string variant;                //< if set, specifies to inject a #ifdef statemnet
		std::string arrayConst;             //< if set, specifies that this type is an array set by a constant value
		uint32_t arraySize = UINT32_MAX;    //< if not uint32_max, indicates the type is an array
	};

	/// uniform buffers
	struct BufferDescriptor
	{
		Descriptor descr;
		std::vector<Descriptor> data;
	};

	/// Specialisation constants
	struct ConstantDescriptor
	{
		std::string name;
		std::string type;
		std::string value;
	};

	/// push constants
	struct PConstantDescriptor
	{
		std::string name;
		std::string type;
		std::string id;
		std::vector<Descriptor> data;
	};

private:
	// points to the next shader stage
	ShaderDescriptor* nextStage = nullptr;

	// shader stage
	Shader::StageType type;

	// sementic inputs and outputs
	std::vector<Descriptor> inputs;
	std::vector<Descriptor> outputs;

	// texture samplers to import; first: name, second: sampler type
	std::vector<Descriptor> samplers;

	// uniform buffers to import; first: name, second: buffer type
	std::vector<BufferDescriptor> ubos;

	// first: name, second: type, third: value
	std::vector<ConstantDescriptor> constants;
	std::vector<PConstantDescriptor> pConstants;

	// the glsl code in text format
	std::vector<std::string> code;

	// variants for this stage
	GlslCompiler::VariantMap variants;

	// used by the compiler to prepare the code block for inputs, etc.
	std::string appendBlock;
};

/**
 * Raw data obtained from a json sampler file.
 */
class ShaderParser
{
public:
	ShaderParser()
	{
	}

	/**
     * @brief Loads a shader json file into a string buffer and parses the json file to extract all data ready for compiling
     * @param filename The path to the shader json to load
     * @param output The string buffer in which the json file will be contained within
     */
	bool parse(Util::String filename);

	/**
	* @brief Takes a empty shader descriptor and parses form the specified json file and shader type, the relevant data
	* This function is solely used when using wanting to merge different shader stages from different files.
	* Usually this will be cached with the shader manager.
	*/
	bool prepareShader(Util::String filename, ShaderDescriptor* shader, Shader::StageType type);

private:
	bool parseShaderJson();
	bool readShader(rapidjson::Document& doc, ShaderDescriptor& shader, std::string id, uint16_t& maxGroup);

	friend class ShaderCompiler;

private:
	rapidjson::Document& doc;

	// used to work out the maximum set number across all stages
	uint16_t groupSize = 0;

	// This will be completed by the parser and ownership moved at compile time.
	// This is to stop having to compile this info twice when it can be easily done by the parser
	std::unique_ptr<RenderStateBlock> renderState;

	// A vertex shader is mandatory
	std::unique_ptr<ShaderDescriptor> vertShader;

	// all other shaders are optional
	std::unique_ptr<ShaderDescriptor> tessEvalShader;
	std::unique_ptr<ShaderDescriptor> tessCompShader;
	std::unique_ptr<ShaderDescriptor> geomShader;
	std::unique_ptr<ShaderDescriptor> fragShader;

	// input buffer
	std::string buffer;
};

/**
 * Compiles a parsed shder json file into data ready for inputting into the renderer
 */
class ShaderCompiler
{
public:
	ShaderCompiler(ShaderProgram& program);
	~ShaderCompiler();

	bool compile(ShaderParser& parser);

	bool compileStage(ShaderDescriptor* shader);

private:
	void prepareBindings(ShaderDescriptor* shader, uint16_t& bind);

	void writeInputs(ShaderDescriptor* shader, ShaderDescriptor* nextShader);

	void prepareInputs(ShaderDescriptor* vertShader);

	void prepareOutputs(ShaderParser& compilerInfo);

private:
	/// variants to use when compiling the shader
	std::unordered_map<std::string, uint8_t> variants;

	/// the program which will be compiled too
	ShaderProgram& program;
};

/**
 * @brief All the data obtained when compiling the glsl shader json file
 */
class ShaderProgram
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
		Shader::StageType shader;
	};

	/**
     * The sampler bindings for the shder stage. These can be sampler2D, sampler3D and samplerCube
     */
	struct SamplerBinding
	{
		Util::String name;
		int16_t bind = 0;
		uint16_t set = 0;
		Shader::StageType shader;
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

	ShaderProgram() = default;

	/**
	* @brief Compiles the parsed json file into data that can be used by the vulkan backend
	* Note: You must have parsed the json file by calling **parse** before calling this function
	* otherwise it will return an error
	*/
	bool prepare(ShaderParser& parser);

	/**
	* @brief Compiles a specific shader stage.This is usually useful when taking stages from different files and merging into
	* shader program.
	*/
	void prepareStage(ShaderDescriptor* descriptor);

	/**
     * @brief Adds a shader variant for a specifed stage to the list
     */
	void addVariant(Util::String definition, uint8_t value, Shader::StageType stage);

	void overrideRenderState(RenderStateBlock* renderState);

	/**
     * @brief Updates a spec constant which must have been stated in the shader json with a new value which will
     * set at pipeline generation time. If the spec constant isn't uodated, then the const value stated in the json will be used
     * Note: only integers and floats supported at present
     */
	void updateConstant(Util::String name, uint32_t value, Shader::StageType stage);
	void updateConstant(Util::String name, int32_t value, Shader::StageType stage);
	void updateConstant(Util::String name, float value, Shader::StageType stage);


	friend class ShaderCompiler;

private:
	std::vector<BufferBinding> bufferBindings;
	std::vector<SamplerBinding> samplerBindings;
	std::vector<RenderTarget> renderTargets;
	std::vector<InputBinding> inputs;

	// Specialisation constant are finalised at the pipeline creation stage
	std::vector<SpecConstantBinding> constants;

	// this block overrides all render state for this shader.
	std::unique_ptr<RenderStateBlock> renderState;

	// We need a layout for each group
	std::vector<DescriptorLayout> descrLayouts;
	PipelineLayout pLineLayout;
};


class ShaderManager
{
public:
	ShaderManager(VkDriver& context);
	~ShaderManager();

	/**
     * @brief Creates a new shader program instance. This will be inserted into the map.
	 * @param name The name of the shader to find - the filename
     * @param renderBlock Whether this shader has a render override block
     * @param variantBits The variant flags used by this shader
	 * @return A pointer to the newly created shader program
     */
	ShaderProgram* createNewInstance(Util::String name, RenderStateBlock* renderBlock, uint64_t variantBits);

	/**
     * @brief Checks whether a shader has been created based on the hash
     * @param name The name of the shader to find - the filename
     * @param renderBlock Whether this shader has a render override block
     * @param variantBits The variant flags used by this shader
     * @return A boolean set to true if the shader exsists, otherwise false
     */
	bool hasShaderVariant(Util::String name, RenderStateBlock* renderBlock, uint64_t variantBits);

	/**
	* @brief Creates a shader fragment that will be cached until ready for use
	*/
	ShaderDescriptor* createCachedInstance(Util::String name, RenderStateBlock* renderBlock, uint64_t variantBits);

	/**
	* @brief Checks whether a shader fragment has been cached as specified by the hash
	*/
	bool hasShaderVariantCached(Util::String name, RenderStateBlock* renderBlock, uint64_t variantBits);


private:
	// =============== shader hasher ======================
	struct ShaderHash
	{
		const char* name;
		uint64_t variantBits;
		vk::PrimitiveTopology* topology;    //< optional (leave null if not needed)
	};

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
