#include "ProgramManager.h"

#include "Utility/logger.h"
#include "utility/FileUtil.h"

#include "VulkanAPI/Compiler/ShaderCompiler.h"
#include "VulkanAPI/Compiler/ShaderParser.h"

#include "VulkanAPI/Sampler.h"
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

bool ShaderProgram::addDescrSetUpdateInfo(Util::String id, Sampler& sampler, ImageView& imageView, vk::ImageLayout layout, Shader::Type stage)
{
    ShaderBinding& binding = stages[stage];
    for (const auto& bind : binding.samplerBindings)
    {
        if (bind.name.compare(id))
        {
            bind.imageView = &imageView;
            bind.sampler = &sampler;
            return true;
        }
    }
    // we shouldn't be here if everything went OK!
    LOGGER_ERROR("Unable to find a buffer with binding id: %s.", id.c_str());
    return false;
}

bool ShaderProgram::addDescrSetUpdateInfo(Util::String id, Buffer& buffer, vk::ImageLayout layout, Shader::Type stage)
{
    ShaderBinding& binding = stages[stage];
    for (const auto& bind : binding.bufferBindings)
    {
        if (bind.name.compare(id))
        {
            bind.buffer = &buffer;
            return true;
        }
    }
    // we shouldn't be here if everything went OK!
    LOGGER_ERROR("Unable to find a sampler with binding id: %s.", id.c_str());
    return false;
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

ShaderProgram* ProgramManager::build(std::vector<ShaderHash>& hashes)
{
	if (hashes.empty())
	{
		return nullptr;
	}

	// ids for the new hash
	uint64_t mergedVariants = 0;
	const char* instanceName;    // we use the vertex name as the identifing id
	vk::PrimitiveTopology* topo;

	ShaderParser parser;

	ShaderProgram* instance = new ShaderProgram();
	for (const ShaderHash& hash : hashes)
	{
		ShaderDescriptor* descr = findCachedVariant(hash);
		if (!descr)
		{
			LOGGER_ERROR("Unable to find cached shader varinat with id: %s and varinat id: %lu", hash.name,
			             hash.variantBits);
			return nullptr;
		}
		else if (descr->type == Shader::Type::Vertex)
		{
			instanceName = descr->id;
			topo = hash.topology;
		}
		parser.addStage(descr);
	}

	// use the render state from the mesh/material
	instance->overrideRenderState();

	// compile
	ShaderCompiler compiler(*instance, context.getContext());
	compiler.compile(parser);

	ShaderHash newHash{ instanceName, mergedVariants, topo };
	programs.emplace(newHash, instance);
	return instance;
}

ShaderProgram* ProgramManager::createNewInstance(ShaderHash& hash)
{
	ShaderProgram* instance = new ShaderProgram();

	programs.emplace(hash, instance);
	return instance;
}

ShaderProgram* ProgramManager::findVariant(ShaderHash& hash)
{
	auto iter = programs.find(hash);
	if (iter == programs.end())
	{
		return iter->second;
	}
	return nullptr;
}

bool ProgramManager::hasShaderVariant(ShaderHash& hash)
{
	auto iter = programs.find(hash);
	if (iter == programs.end())
	{
		return false;
	}
	return true;
}

ShaderDescriptor* ProgramManager::createCachedInstance(ShaderHash& hash)
{
	ShaderDescriptor* instance = new ShaderDescriptor();

	cached.emplace(hash, instance);
	return instance;
}

bool ProgramManager::hasShaderVariantCached(ShaderHash& hash)
{
	auto iter = cached.find(hash);
	if (iter == cached.end())
	{
		return false;
	}
	return true;
}

}    // namespace VulkanAPI
