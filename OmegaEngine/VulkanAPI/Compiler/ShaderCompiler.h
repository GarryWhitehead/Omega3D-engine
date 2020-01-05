#pragma once

#include <cstdint>
#include <unordered_map>

namespace VulkanAPI
{
// forward declerations
class ShaderDescriptor;
class ShaderProgram;
class ShaderBinding;
class ShaderParser;
class VkContext;

    /**
 * Compiles a parsed shder json file into data ready for inputting into the renderer
 */
    class ShaderCompiler
{
public:
	ShaderCompiler(ShaderProgram& program, VkContext& context);
	~ShaderCompiler();

	bool compile(ShaderParser& parser);

	bool compileStage(ShaderDescriptor* shader);

private:
	void prepareBindings(ShaderDescriptor* shader, ShaderBinding& binding, uint16_t& bind);

	void writeInputs(ShaderDescriptor* shader, ShaderDescriptor* nextShader);

	void prepareInputs(ShaderDescriptor* vertShader);

	void prepareOutputs(ShaderParser& compilerInfo, ShaderBinding& binding);

private:
	VkContext& context;

	/// variants to use when compiling the shader
	std::unordered_map<const char*, uint8_t> variants;

	/// the program which will be compiled too
	ShaderProgram& program;
};

}    // namespace VulkanAPI
