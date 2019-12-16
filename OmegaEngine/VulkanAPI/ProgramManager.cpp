#include "ProgramManager.h"

#include "Utility/logger.h"
#include "utility/FileUtil.h"

#include "VulkanAPI/Compiler/ShaderCompiler.h"
#include "VulkanAPI/Compiler/ShaderParser.h"

#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Sampler.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/VkTexture.h"
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

std::vector<vk::PipelineShaderStageCreateInfo> ShaderProgram::getShaderCreateInfo()
{
	// probably something wrong if your calling this with no stages registered
	assert(!stages.empty());

	std::vector<vk::PipelineShaderStageCreateInfo> createInfos;
	for (const ShaderBinding& bind : stages)
	{
		createInfos.emplace_back(bind.shader.createInfo);
	}
	return createInfos;
}

bool ShaderProgram::updateDecrSetBuffer(Util::String id, Buffer& buffer)
{
	for (auto& stage : stages)
	{
		for (auto& binding : stage.bufferBindings)
		{
			if (binding.name.compare(id))
			{
				descrSet->updateBufferSet(binding.set, binding.bind, buffer.getType(), buffer, 0, buffer.getSize());
				return true;
			}
		}
	}
	return false;
}

bool ShaderProgram::updateDecrSetImage(Util::String id, Texture& tex)
{
	for (auto& stage : stages)
	{
		for (auto& binding : stage.bufferBindings)
		{
			if (binding.name.compare(id))
			{
				descrSet->updateImageSet(binding.set, binding.bind, binding.type, tex.getSampler(), tex.getImageView(),
				                         tex.getLayout());
				return true;
			}
		}
	}
	return false;
}

ShaderBinding::SamplerBinding& ShaderProgram::findSamplerBinding(Util::String name, const Shader::Type type)
{
	auto iter = std::find(stages.begin(), stages.end(),
	                      [](const ShaderBinding& lhs, const ShaderBinding& rhs) { return lhs.type == rhs.type; });

	assert(iter != stages.end());
	for (auto& sampler : iter->samplerBindings)
	{
		if (sampler.name.compare(name))
		{
			return sampler;
		}
	}
	// a binding with this name doesn't exsist
	throw std::runtime_error("Unable to find a sampler binding with id: %s", name.c_str());
}

DescriptorLayout* ShaderProgram::getDescrLayout()
{
	return descrLayout.get();
}

DescriptorSet* ShaderProgram::getDescrSet()
{
	return descrSet.get();
}

// =================== Program Manager ==================
ProgramManager::ProgramManager(VkDriver& driver)
    : driver(driver)
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
	for (ShaderHash& hash : hashes)
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
	ShaderCompiler compiler(*instance, driver.getContext());
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

ShaderDescriptor* ProgramManager::findCachedVariant(ShaderHash& hash)
{
	auto iter = cached.find(hash);
	if (iter == cached.end())
	{
		return &iter->second;
	}
	return nullptr;
}

void ProgramManager::pushBufferDescrUpdate(Util::String id, Buffer& buffer)
{
	assert(!id.empty());
	bufferDescrQueue.emplace_back(std::make_pair(id, buffer));
}

void ProgramManager::pushImageDescrUpdate(Util::String id, Texture& tex)
{
	assert(!id.empty());
	imageDescrQueue.emplace_back(std::make_pair(id, tex));
}

void ProgramManager::pushMatDescrUdpdate(Util::String id, DescriptorSet* set)
{
	assert(set);
	assert(!id.empty());
	matDescrQueue.emplace_back(std::make_pair(id, set));
}

void ProgramManager::updateBufferDecsrSets()
{
	if (bufferDescrQueue.empty())
	{
		return;
	}

	for (auto& queuePair : bufferDescrQueue)
	{
		bool result = false;
		for (auto& prog : programs)
		{
			result &= prog.second->updateDecrSetBuffer(queuePair.first, queuePair.second);
		}
		if (!result)
		{
			throw std::runtime_error("Unable to find buffer with id %s.", queuePair.first.c_str());
		}
	}

	// all done, so clear the updates ready for the next frame
	bufferDescrQueue.clear();
}

void ProgramManager::updateImageDecsrSets()
{
	if (imageDescrQueue.empty())
	{
		return;
	}

	for (auto& queuePair : imageDescrQueue)
	{
		bool result = false;
		for (auto& prog : programs)
		{
			result &= prog.second->updateDecrSetImage(queuePair.first, queuePair.second);
		}
		if (!result)
		{
			throw std::runtime_error("Unable to find buffer with id %s.", queuePair.first.c_str());
		}
	}

	// all done, so clear the updates ready for the next frame
	imageDescrQueue.clear();
}

}    // namespace VulkanAPI
