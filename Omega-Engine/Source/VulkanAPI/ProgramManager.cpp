#include "ProgramManager.h"

#include "VulkanAPI/Compiler/ShaderCompiler.h"
#include "VulkanAPI/Compiler/ShaderParser.h"
#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/Sampler.h"
#include "VulkanAPI/VkContext.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/VkTexture.h"
#include "VulkanAPI/VkUtils/StringToVk.h"
#include "VulkanAPI/VkUtils/VkToString.h"
#include "utility/FileUtil.h"
#include "utility/Logger.h"

#include <stdexcept>

namespace VulkanAPI
{

// =================== ShaderProgram ======================

ShaderProgram::ShaderProgram(VkContext& context)
    : context(context)
    , pLineLayout(std::make_unique<PipelineLayout>())
{
}

void ShaderProgram::addVariant(Util::String& definition, uint8_t value, Shader::Type stage)
{
    variants.emplace_back(VulkanAPI::Shader::VariantInfo {definition, value, stage});
}

void ShaderProgram::addVariants(GlslCompiler::VariantMap& map, const Shader::Type type)
{
    for (auto& def : map)
    {
        addVariant(Util::String(def.first), def.second, type);
    }
}

void ShaderProgram::overrideRenderState(RenderStateBlock* renderState)
{
    // TODO
}

bool ShaderProgram::prepare(ShaderParser& parser)
{
    ShaderCompiler compiler(*this, context);

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
        createInfos.emplace_back(bind.shader->createInfo);
    }
    return createInfos;
}

ShaderBinding::SamplerBinding&
ShaderProgram::findSamplerBinding(Util::String name, const Shader::Type type)
{
    auto iter = std::find_if(stages.begin(), stages.end(), [&type](const ShaderBinding& lhs) {
        return lhs.shader->type == type;
    });

    assert(iter != stages.end());
    for (auto& sampler : iter->samplerBindings)
    {
        if (sampler.name == std::string(name.c_str()))
        {
            return sampler;
        }
    }
    // a binding with this name doesn't exsist
    throw std::runtime_error("Unable to find sampler binding");
}

ShaderBinding& ShaderProgram::findShaderBinding(const Shader::Type type)
{
    auto iter = std::find_if(stages.begin(), stages.end(), [&type](const ShaderBinding& lhs) {
        return lhs.shader->type == type;
    });

    if (iter != stages.end())
    {
        return *iter;
    }
    throw std::runtime_error("Unable to find shader binding");
}

std::vector<VulkanAPI::Shader::VariantInfo> ShaderProgram::sortVariants(Shader::Type stage)
{
    std::vector<VulkanAPI::Shader::VariantInfo> ret;
    for (auto& variant : variants)
    {
        if (variant.stage == stage)
        {
            ret.emplace_back(variant);
        }
    }
    return ret;
}

DescriptorLayout* ShaderProgram::getDescrLayout(uint8_t set)
{
    return &descrPool->findSet(set);
}

DescriptorSet* ShaderProgram::getDescrSet()
{
    return descrSet.get();
}

PipelineLayout* ShaderProgram::getPLineLayout()
{
    return pLineLayout.get();
}

// =================== Program Manager ==================
ProgramManager::ProgramManager(VkContext& context) : context(context)
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

ShaderProgram* ProgramManager::build(ShaderParser& parser, std::vector<ShaderKey>& hashes)
{
    if (hashes.empty())
    {
        return nullptr;
    }

    // ids for the new hash
    uint64_t mergedVariants = 0;
    const char* instanceName; // we use the vertex name as the identifing id
    uint32_t topo;

    ShaderProgram* instance = new ShaderProgram(context);
    for (ShaderKey& hash : hashes)
    {
        ShaderDescriptor* descr = findCachedVariant(hash);
        if (!descr)
        {
            LOGGER_ERROR(
                "Unable to find cached shader varinat with id: %s and varinat id: %llu",
                hash.name,
                hash.variantBits);
            return nullptr;
        }
        else if (descr->type == Shader::Type::Vertex)
        {
            instanceName = hash.name;
            topo = hash.topology;
        }
        if (!parser.addStage(*descr))
        {
            LOGGER_ERROR("Error wilst building shader program. This is likely due to trying to add "
                         "the same stage twice.");
            return nullptr;
        }
    }

    // use the render state from the mesh/material
    // instance->overrideRenderState();

    ShaderKey newHash {instanceName, mergedVariants, topo};
    programs.emplace(newHash, instance);
    return instance;
}

bool ProgramManager::compile(ShaderParser& parser, ShaderProgram* prog)
{
    ShaderCompiler compiler(*prog, context);
    if (!compiler.compile(parser))
    {
        printf("Fatal Error: %s\n", compiler.getErrorString().c_str());
        return false;
    }
    return true;
}

ShaderProgram* ProgramManager::createNewInstance(ShaderKey& hash)
{
    ShaderProgram* instance = new ShaderProgram(context);

    programs.emplace(hash, instance);
    return instance;
}

ShaderProgram* ProgramManager::findVariant(ShaderKey& hash)
{
    auto iter = programs.find(hash);
    if (iter != programs.end())
    {
        return iter->second;
    }
    return nullptr;
}

bool ProgramManager::hasShaderVariant(ShaderKey& hash)
{
    auto iter = programs.find(hash);
    if (iter == programs.end())
    {
        return false;
    }
    return true;
}

ShaderDescriptor* ProgramManager::createCachedInstance(
    ShaderKey& hash, ShaderDescriptor& descr, const Shader::Type type)
{
    cached.emplace(hash, descr);
    return &cached[hash];
}

bool ProgramManager::hasShaderVariantCached(ShaderKey& hash)
{
    auto iter = cached.find(hash);
    if (iter == cached.end())
    {
        return false;
    }
    return true;
}

ShaderDescriptor* ProgramManager::findCachedVariant(ShaderKey& hash)
{
    auto iter = cached.find({hash});
    if (iter != cached.end())
    {
        return &iter->second;
    }
    return nullptr;
}

void ProgramManager::pushBufferDescrUpdate(Util::String id, Buffer& buffer)
{
    assert(!id.empty());
    bufferDescrQueue.emplace_back(std::make_pair(id.c_str(), &buffer));
}

void ProgramManager::pushImageDescrUpdate(Util::String id, Texture& tex)
{
    assert(!id.empty());
    imageDescrQueue.emplace_back(std::make_pair(id.c_str(), &tex));
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
            result &= prog.second->updateDecrSetBuffer(queuePair.first, *queuePair.second);
        }
        if (!result)
        {
            throw std::runtime_error("Unable to find buffer");
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
            result &= prog.second->updateDecrSetImage(queuePair.first, *queuePair.second);
        }
        if (!result)
        {
            throw std::runtime_error("Unable to find buffer");
        }
    }

    // all done, so clear the updates ready for the next frame
    imageDescrQueue.clear();
}

ShaderProgram* ProgramManager::getVariant(ProgramManager::ShaderKey& key)
{
    VulkanAPI::ShaderProgram* prog = nullptr;

    if (!hasShaderVariant(key))
    {
        VulkanAPI::ShaderParser parser;
        if (!parser.loadAndParse(key.name))
        {
            printf("Fatal Error: %s; shader name: %s\n", parser.getErrorString().c_str(), key.name);
            return nullptr;
        }
        prog = createNewInstance(key);

        // add variants and constant values

        assert(prog);
        if (!prog->prepare(parser))
        {
            return nullptr;
        }
    }
    else
    {
        prog = findVariant(key);
    }
    return prog;
}

} // namespace VulkanAPI
