#include "ProgramManager.h"

#include "Utility/logger.h"
#include "utility/FileUtil.h"

#include "VulkanAPI/Compiler/ShaderParser.h"
#include "VulkanAPI/Compiler/ShaderCompiler.h"

#include "VulkanAPI/Shader.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/VkUtils/StringToVk.h"
#include "VulkanAPI/VkUtils/VkToString.h"

namespace VulkanAPI
{

// =================== ShaderProgram ======================

void ShaderProgram::addVariant(Util::String definition, uint8_t value, Shader::Type stage)
{
}

void ShaderProgram::overrideRenderState(RenderStateBlock* renderState)
{
}

void ShaderProgram::updateConstant(Util::String name, uint32_t value, Shader::Type stage)
{
}

void ShaderProgram::updateConstant(Util::String name, int32_t value, Shader::Type stage)
{
}

void ShaderProgram::updateConstant(Util::String name, float value, Shader::Type stage)
{
}

bool ShaderProgram::prepare(ShaderParser& parser, VkContext& context)
{
	ShaderCompiler compiler(*this, context);

	// create a variation of the shader if variants are specfied
	if (!variants.empty())
	{
		compiler.addVariant(variants);
	}

	if (!compiler.compile(parser))
	{
		return false;
	}
	return true;
}

// =================== Shader Manager ==================
ProgramManager::ProgramManager(VkDriver& context)
    : context(context)
{
}

ProgramManager::~ProgramManager()
{
	for (auto& program : programs)
	{
		if (program.second)
		{
			delete program.second;
		}
	}
}

ShaderProgram* ProgramManager::createNewInstance(Util::String name, RenderStateBlock* renderBlock, uint64_t variantBits)
{
	ShaderProgram* instance = new ShaderProgram();

	ShaderHash hash{ name.c_str(), variantBits, renderBlock };
	programs.emplace(hash, instance);
	return instance;
}

bool ProgramManager::hasShaderVariant(Util::String name, RenderStateBlock* renderBlock, uint64_t variantBits)
{
	ShaderHash hash{ name.c_str(), variantBits, renderBlock };
	auto iter = programs.find(hash);
	if (iter == programs.end())
	{
		return false;
	}
	return true;
}

ShaderDescriptor* ProgramManager::createCachedInstance(Util::String name, RenderStateBlock* renderBlock,
                                                       uint64_t variantBits)
{
	ShaderDescriptor* instance = new ShaderDescriptor();

	ShaderHash hash{ name.c_str(), variantBits, renderBlock };
	cached.emplace(hash, instance);
	return instance;
}

bool ProgramManager::hasShaderVariantCached(Util::String name, RenderStateBlock* renderBlock, uint64_t variantBits)
{
	ShaderHash hash{ name.c_str(), variantBits, renderBlock->rastState };
	auto iter = cached.find(hash);
	if (iter == cached.end())
	{
		return false;
	}
	return true;
}

}    // namespace VulkanAPI
