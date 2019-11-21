#pragma once

#include "VulkanAPI/Common.h"
#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/Pipeline.h"

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
		ShaderType shader;
	};

	/**
     * The sampler bindings for the shder stage. These can be sampler2D, sampler3D and samplerCube
     */
	struct SamplerBinding
	{
		Util::String name;
		int16_t bind = 0;
		uint16_t set = 0;
		ShaderType shader;
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

	/**
     * States the ouput from the fragment shader - into a buffer specified by the render pass
     */
	struct RenderTarget
	{
		uint16_t location = 0;
		vk::Format format;
	};

	ShaderProgram() = default;

	friend class ShaderCompiler;

private:
	std::vector<BufferBinding> bufferBindings;
	std::vector<SamplerBinding> samplerBindings;
	std::vector<RenderTarget> renderTargets;
	std::vector<InputBinding> inputs;

	// We need a layout for each group
	std::vector<DescriptorLayout> descrLayouts;
	PipelineLayout pLineLayout;
	Pipeline pipeline;
};

/**
* @brief Specifies the render pipeline state of the shader program
* Can also be passed explicitly to override pre-exsisting data
*/
struct RenderStateBlock
{
	struct DepthStencilState
	{
		bool testEnable = false;
		bool writeEnable = false;
		vk::CompareOp compareOp = vk::CompareOp::eLessOrEqual;
	};

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

	DepthStencilState dsState;
	RasterState rastState;
	Sampler sampler;
};

/**
 * Raw data obtained from a json sampler file.
 */
class ShaderParser
{
public:
	struct ShaderDescriptor
	{
		ShaderDescriptor(Shader::StageType type)
		    : type(type)
		{
		}

		struct Descriptor
		{
			std::string name;
			std::string type;
			uint16_t groupId;
		};

		struct BufferDescriptor
		{
			Descriptor descr;
			std::vector<Descriptor> data;
		};

		struct ConstantDescriptor
		{
			Descriptor descr;
			std::string value;
		};

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

		// the glsl code in text format
		std::vector<std::string> code;

		// used by the compiler to prepare the code block for inputs, etc.
		std::string appendBlock;
	};


	ShaderParser() = default;

	/**
     * @brief Loads a shader json file into a string buffer and parses the json file to extract all data ready for compiling
     * @param filename The path to the shader json to load
     * @param output The string buffer in which the json file will be contained within
     */
	bool parse(Util::String filename);

private:
	bool parseShaderJson();
	bool readShader(rapidjson::Document& doc, ShaderDescriptor& shader, std::string id, uint16_t& maxGroup);

	friend class ShaderCompiler;

private:
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
	void addVariant(Util::String definition, uint8_t value);

	void overrideRenderState(RenderStateBlock& rState);

	bool compile(ShaderParser& parser);

private:
	void prepareBindings(ShaderParser::ShaderDescriptor* shader, uint16_t& bind);

	void writeInputs(ShaderParser::ShaderDescriptor* shader, ShaderParser::ShaderDescriptor* nextShader);

	void prepareInputs(ShaderParser::ShaderDescriptor* vertShader);

	void prepareOutputs(ShaderParser& compilerInfo);

private:
	/// variants to use when compiling the shader
	std::unordered_map<std::string, uint8_t> variants;

	/// Overide the render data with the user defined version
	std::unique_ptr<RenderStateBlock> renderState;

	ShaderProgram program;
};

class ShaderManager
{
public:
	ShaderManager(VkDriver& context);
	~ShaderManager();

	/**
     * @brief If a shader already exsists with a identical hash then that shder will be returned
     * @param name The filename of the shader json - also used for the hash
     * @param renderBlock optional render override block which will be used as rendering data for the shader
     * instead of the data extracted from the json file - also used for the hash
     * @param variantBits The varaiant flags - used for the hash
     * @param variantData A optional pointer to an array of variant data
     * @param variantSize A optional size parameter indicating the number of variants contained in **variantData**
     */
	ShaderProgram* findOrCreateShader(Util::String name, RenderStateBlock* renderBlock, uint64_t variantBits,
	                                  VariantInfo* variantData = nullptr, uint32_t variantSize = 0);

	/**
     * @brief Checks whether a shader has been created based on the hash
     * @param name The name of the shader to find - the filename
     * @param renderBlock Wether this shader has a render override block
     * @param variantBits The variant flags used by this shader
     * @return A boolean set to true if the shader exsists, otherwise false
     */
	bool hasShader(Util::String name, RenderStateBlock* renderBlock, uint64_t variantBits);

private:
	// =============== shader hasher ======================
	struct ShaderHash
	{
		const char* name;
		uint64_t variantBits;
	};

	struct ShaderHasher
	{
		size_t operator()(ShaderHash const& id) const noexcept
		{
			size_t h1 = std::hash<const char*>{}(id.name);
			size_t h2 = std::hash<uint64_t>{}(id.variantBits);
			return h1 ^ (h2 << 1);
		}
	};

	struct ShaderEqual
	{
		bool operator()(const ShaderHash& lhs, const ShaderHash& rhs) const
		{
			return lhs.name == rhs.name && lhs.variantBits == rhs.variantBits;
		}
	};

private:
	VkDriver& context;

	std::unordered_map<ShaderHash, ShaderProgram, ShaderHasher, ShaderEqual> programs;
};

}    // namespace VulkanAPI
