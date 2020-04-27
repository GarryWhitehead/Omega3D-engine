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

#include "ProgramManager.h"

#include "VulkanAPI/Compiler/ShaderCompiler.h"
#include "VulkanAPI/Compiler/ShaderParser.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/VkContext.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/VkTexture.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/VkUtils/StringToVk.h"
#include "VulkanAPI/VkUtils/VkToString.h"
#include "utility/FileUtil.h"
#include "utility/Logger.h"

#include <stdexcept>

namespace VulkanAPI
{

// =================== ShaderProgram ======================

ShaderProgram::ShaderProgram(VkDriver& driver, uint32_t shaderId)
    : driver(driver),
      shaderId(shaderId)
    , pLineLayout(std::make_unique<PipelineLayout>())
{
}

void ShaderProgram::addVariant(const Util::String& definition, uint8_t value, Shader::Type stage)
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
    ShaderCompiler compiler(*this, driver);

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
ShaderProgram::findSamplerBinding(const Util::String& name, const Shader::Type type)
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

std::vector<ShaderBinding::SamplerBinding>& ShaderProgram::getMaterialBindings()
{
    return materialBindings;
}

PipelineLayout* ShaderProgram::getPLineLayout()
{
    return pLineLayout.get();
}

uint8_t ShaderProgram::getSetCount() const
{
    return setCount;
}

uint32_t ShaderProgram::getShaderId() const
{
    return shaderId;
}

// =================== Program Manager ==================

ProgramManager::ProgramManager(VkDriver& driver) : driver(driver)
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

ShaderProgram* ProgramManager::build(ShaderParser& parser, const std::vector<CachedKey>& hashes)
{
    if (hashes.empty())
    {
        return nullptr;
    }

    // ids for the new hash
    uint64_t mergedVariants = 0;
    uint32_t instanceId;
    uint32_t topo;
    
    assert(hashes[0].shaderId);
    ShaderProgram* instance = new ShaderProgram(driver, hashes[0].shaderId);
    for (const CachedKey& hash : hashes)
    {
        ShaderDescriptor* descr = findCachedVariant(hash);
        if (!descr)
        {
            LOGGER_ERROR(
                "Unable to find cached shader variant with id: %i and variant id: %llu",
                hash.shaderId,
                hash.variantBits);
            return nullptr;
        }
        else if (descr->type == Shader::Type::Vertex)
        {
            instanceId = hash.shaderId;
            topo = hash.topology;
        }
        
        // add the new shader stage to the program
        if (!parser.addStage(*descr))
        {
            LOGGER_ERROR("Error wilst building shader program. This is likely due to trying to add "
                         "the same stage twice.");
            return nullptr;
        }
    }

    // use the render state from the mesh/material
    // instance->overrideRenderState();

    ShaderKey newHash {instanceId, mergedVariants, topo};
    programs.emplace(newHash, instance);
    
    return instance;
}

bool ProgramManager::compile(ShaderParser& parser, ShaderProgram* prog)
{
    ShaderCompiler compiler(*prog, driver);
    if (!compiler.compile(parser))
    {
        printf("Fatal Error: %s\n", compiler.getErrorString().c_str());
        return false;
    }
    return true;
}

ShaderProgram* ProgramManager::createNewInstance(const ShaderKey& hash)
{
    ShaderProgram* instance = new ShaderProgram(driver, hash.shaderId);
    programs.emplace(hash, instance);
    return instance;
}

ShaderProgram* ProgramManager::findVariant(const ShaderKey& hash)
{
    auto iter = programs.find(hash);
    if (iter != programs.end())
    {
        return iter->second;
    }
    return nullptr;
}

bool ProgramManager::hasShaderVariant(const ShaderKey& hash)
{
    auto iter = programs.find(hash);
    if (iter == programs.end())
    {
        return false;
    }
    return true;
}

ShaderDescriptor* ProgramManager::createCachedInstance(
    const CachedKey& hash, const ShaderDescriptor& descr)
{
    cached.emplace(hash, descr);
    return &cached[hash];
}

bool ProgramManager::hasShaderVariantCached(const CachedKey& key)
{
    auto iter = cached.find(key);
    if (iter == cached.end())
    {
        return false;
    }
    return true;
}

ShaderDescriptor* ProgramManager::findCachedVariant(const CachedKey& key)
{
    auto iter = cached.find({key});
    if (iter != cached.end())
    {
        return &iter->second;
    }
    return nullptr;
}

ShaderProgram* ProgramManager::getVariantOrCreate(const Util::String& filename, uint64_t variantBits, uint32_t topology)
{
    VulkanAPI::ShaderProgram* prog = nullptr;
    
    // the shader filename is hashed and used as the id for the key
    uint32_t shaderHash = Util::murmurHash3((const uint32_t*)filename.c_str(), filename.size(), 0);
    
    VulkanAPI::ProgramManager::ShaderKey key = {shaderHash, variantBits, topology};
    
    if (!hasShaderVariant(key))
    {
        VulkanAPI::ShaderParser parser;
        if (!parser.loadAndParse(filename))
        {
            printf("Fatal Error: %s; shader id: %i\n", parser.getErrorString().c_str(), key.shaderId);
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
