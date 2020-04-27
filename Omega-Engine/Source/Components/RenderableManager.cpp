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

#include "RenderableManager.h"

#include "Components/TransformManager.h"
#include "Core/engine.h"
#include "ModelImporter/MeshInstance.h"
#include "OEMaths/OEMaths_transform.h"
#include "VulkanAPI/CBufferManager.h"
#include "VulkanAPI/Compiler/ShaderParser.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/Utility.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/VkTexture.h"
#include "utility/GeneralUtil.h"
#include "utility/Logger.h"
#include "utility/MurmurHash.h"


namespace OmegaEngine
{
// =================================================================================
// User interface renderable building functions

RenderableInstance& RenderableInstance::addMesh(MeshInstance* instance)
{
    assert(instance);
    mesh = instance;
    return *this;
}

RenderableInstance& RenderableInstance::addMaterial(MaterialInstance* instance)
{
    assert(instance);
    mat = instance;
    return *this;
}

RenderableInstance& RenderableInstance::addSkin(SkinInstance* instance)
{
    skin = instance;
    return *this;
}

RenderableInstance& RenderableInstance::addNode(NodeInstance* instance)
{
    assert(instance);
    node = instance;
    return *this;
}

void RenderableInstance::create(Engine& engine, Object* obj)
{
    OEEngine& oeEngine = reinterpret_cast<OEEngine&>(engine);

    if (!obj)
    {
        LOGGER_WARN("Trying to create a renderable instance though the object is nullptr.");
        return;
    }

    // sanity checks - must have mesh and transform data at the minimum
    if (!mesh || !node)
    {
        LOGGER_WARN("You have called create but this instance has no mesh or transform data "
                    "associated with it.");
        return;
    }

    OERenderableManager* rManager = oeEngine.getRendManager();
    TransformManager& tManager = oeEngine.getTransManager();

    // we don't have to check whether the material or skin in init, the managers will do that
    rManager->addRenderable(mesh, mat, *static_cast<OEObject*>(obj));
    tManager.addNodeHierachy(*node, *static_cast<OEObject*>(obj), skin);
}

// =================================================================================
Util::String TextureGroup::texTypeToStr(const int type)
{
    assert(type < static_cast<int>(MaterialInstance::TextureType::Count));
    Util::String result;
    switch (type)
    {
        case MaterialInstance::TextureType::BaseColour:
            result = "BaseColour";
            break;
        case MaterialInstance::TextureType::Emissive:
            result = "Emissive";
            break;
        case MaterialInstance::TextureType::MetallicRoughness:
            result = "MetallicRoughness";
            break;
        case MaterialInstance::TextureType::Normal:
            result = "Normal";
            break;
        case MaterialInstance::TextureType::Occlusion:
            result = "Occlusion";
            break;
    }
    return result;
}

// ===============================================================================================

void Material::addVariant(const MaterialInstance::TextureType type)
{
    switch (type)
    {
        case MaterialInstance::TextureType::BaseColour:
            variantBits |= Variants::HasBaseColour;
            break;
        case MaterialInstance::TextureType::Normal:
            variantBits |= Variants::HasNormal;
            break;
        case MaterialInstance::TextureType::MetallicRoughness:
            variantBits |= Variants::HasMetallicRoughness;
            break;
        case MaterialInstance::TextureType::Emissive:
            variantBits |= Variants::HasEmissive;
            break;
        case MaterialInstance::TextureType::Occlusion:
            variantBits |= Variants::HasOcclusion;
            break;
        default:
            LOGGER_WARN("Invalid material variant bit. Ingnoring....");
    }
}

VulkanAPI::GlslCompiler::VariantMap
Material::createVariants(Util::BitSetEnum<Material::Variants>& bits)
{
    VulkanAPI::GlslCompiler::VariantMap map;
    if (bits.testBit(HasBaseColour))
    {
        map.emplace("HAS_BASECOLOUR", 1);
    }
    if (bits.testBit(HasNormal))
    {
        map.emplace("HAS_NORMAL", 1);
    }
    if (bits.testBit(HasUv))
    {
        map.emplace("HAS_UV", 1);
    }
    if (bits.testBit(HasMetallicRoughness))
    {
        map.emplace("HAS_METALLICROUGHNESS", 1);
    }
    if (bits.testBit(HasOcclusion))
    {
        map.emplace("HAS_OCCLUSION", 1);
    }
    if (bits.testBit(HasEmissive))
    {
        map.emplace("HAS_EMISSIVE", 1);
    }
    return map;
}

VulkanAPI::GlslCompiler::VariantMap
Renderable::createVariants(Util::BitSetEnum<MeshInstance::Variant>& bits)
{
    VulkanAPI::GlslCompiler::VariantMap map;

    if (bits.testBit(MeshInstance::Variant::HasSkin))
    {
        map.emplace("HAS_SKIN", 1);
    }
    if (bits.testBit(MeshInstance::Variant::BiTangentInput))
    {
        map.emplace("HAS_BITANGENT", 1);
    }
    if (bits.testBit(MeshInstance::Variant::HasJoint))
    {
        map.emplace("HAS_BONES", 1);
    }
    if (bits.testBit(MeshInstance::Variant::HasNormal))
    {
        map.emplace("HAS_NORMAL", 1);
    }
    if (bits.testBit(MeshInstance::Variant::HasWeight))
    {
        map.emplace("HAS_WEIGHTS", 1);
    }
    if (bits.testBit(MeshInstance::Variant::TangentInput))
    {
        map.emplace("HAS_TANGENT", 1);
    }
    return map;
}
// ===============================================================================================

OERenderableManager::OERenderableManager(OEEngine& engine) : engine(engine)
{
    // for performance purposes
    renderables.reserve(MESH_INIT_CONTAINER_SIZE);
}

OERenderableManager::~OERenderableManager()
{
}

void OERenderableManager::addMesh(
    Renderable& input, MeshInstance* mesh, const size_t idx, const size_t offset)
{
    // copy the instance - we cant be 100% sure the user will keep the model in scope
    input.instance = new MeshInstance();
    memcpy(input.instance, mesh, sizeof(MeshInstance));

    // now adjust the material index - this is used primarily by the sorting key
    // a value of -1 indicate no materials for this renderable
    if (offset >= 0)
    {
        input.materialId = idx + offset;
    }
}

void OERenderableManager::addMesh(
    Renderable& input, MeshInstance* mesh, OEObject& obj, const size_t offset)
{
    ObjectHandle handle = addObject(obj);
    addMesh(input, mesh, handle.get(), offset);
}

MappedTexture* OERenderableManager::prepareTexture(Util::String& path)
{
    MappedTexture* tex = nullptr;

    if (!path.empty())
    {
        tex = new MappedTexture();
        assert(tex);

        if (!tex->load(path))
        {
            return nullptr;
        }
    }
    // it's not an error if the path is empty.
    return tex;
}

size_t OERenderableManager::addMaterial(Renderable& input, MaterialInstance* mat)
{
    Material newMat;
    size_t startOffset = materials.size();

    // create a hash of the material name for hash map lookup
    newMat.materialHash = Util::murmurHash3((const uint32_t*) &mat->name, mat->name.size(), 0);

    // sort out the textures
    TextureGroup group;
    group.matName = mat->name;

    for (size_t j = 0; j < MaterialInstance::TextureType::Count; ++j)
    {
        // this function does return an error value but unsure yet
        // whether just to continue (it will be obvious that somethings gone
        // wrong when rendered) or return an error
        group.textures[j] = prepareTexture(mat->texturePaths[j]);
        newMat.addVariant(static_cast<MaterialInstance::TextureType>(j));
    }

    textures.emplace_back(std::move(group));

    // we copy here, as we cant be sure that the user will keep the model in scope
    newMat.instance = new MaterialInstance();
    memcpy(newMat.instance, mat, sizeof(MaterialInstance));

    materials.emplace_back(std::move(newMat));
    input.material = &materials.back();

    return startOffset;
}

void OERenderableManager::addRenderable(MeshInstance* mesh, MaterialInstance* mat, OEObject& obj)
{
    Renderable newRend;

    // first add the object which will give us a free slot
    ObjectHandle handle = addObject(obj);

    // note: materials don't require a slot as there might be more than one material
    // per mesh. Instead the primitive stores a index to the required material
    size_t matOffset = -1;
    if (mat)
    {
        matOffset = addMaterial(newRend, mat);
    }

    // now add the mesh and material and any textures
    addMesh(newRend, mesh, handle.get(), matOffset);

    // check whether we just add to the back or use a freed slot
    if (handle.get() >= renderables.size())
    {
        renderables.emplace_back(newRend);
    }
    else
    {
        renderables[handle.get()] = std::move(newRend);
    }
}

Renderable& OERenderableManager::getMesh(const ObjectHandle& handle)
{
    assert(handle.get() <= renderables.size());
    return renderables[handle.get()];
}

void OERenderableManager::updateBuffers()
{
    VulkanAPI::VkDriver& driver = engine.getVkDriver();

    for (Renderable& rend : renderables)
    {
        if (!rend.vertBuffer && !rend.indexBuffer)
        {
            MeshInstance* instance = rend.instance;
            size_t vertSize = instance->vertices.size;
            size_t indexSize = instance->indices.size() * sizeof(uint32_t);

            rend.vertBuffer = driver.addVertexBuffer(vertSize, instance->vertices.data);
            rend.indexBuffer = driver.addIndexBuffer(indexSize, instance->indices.data());

            assert(rend.vertBuffer && rend.indexBuffer);
        }
    }
}

bool OERenderableManager::updateVariants()
{
    // parse the shader file - this will be used by all variants
    VulkanAPI::ShaderParser parser;

    const Util::String filename = "mrt.glsl";

    if (!parser.loadAndParse(filename))
    {
        printf("Fatal error parsing mrt shader: %s", parser.getErrorString().c_str());
        return false;
    }

    VulkanAPI::ProgramManager& manager = engine.getVkDriver().getProgManager();

    // use a hash of the filename as part of the shader key
    uint32_t shaderHash = Util::murmurHash3((const uint32_t*) filename.c_str(), filename.size(), 0);

    // Note - we try and create as many shader variants as possible for vertex and material
    // shaders as creating them whilst the engine is actually rendering will be costly in terms
    // of performance
    for (const Renderable& rend : renderables)
    {
        vk::PrimitiveTopology topo = VulkanAPI::VkUtil::topologyToVk(rend.instance->topology);
        VulkanAPI::ProgramManager::CachedKey key {shaderHash,
                                                  VulkanAPI::Shader::Type::Vertex,
                                                  rend.instance->variantBits.getUint64(),
                                                  static_cast<uint32_t>(topo)};

        if (!manager.hasShaderVariantCached(key))
        {
            VulkanAPI::ShaderDescriptor* descr =
                parser.getShaderDescriptor(VulkanAPI::Shader::Type::Vertex);
            assert(descr);
            manager.createCachedInstance(key, *descr);
        }
    }

    // ======== create variants required for all materials currently associated with the manager
    for (const Material& mat : materials)
    {
        VulkanAPI::ProgramManager::CachedKey key {
            shaderHash, VulkanAPI::Shader::Type::Fragment, mat.variantBits.getUint64()};
        if (!manager.hasShaderVariantCached(key))
        {
            VulkanAPI::ShaderDescriptor* descr =
                parser.getShaderDescriptor(VulkanAPI::Shader::Type::Fragment);
            assert(descr);
            manager.createCachedInstance(key, *descr);
        }
    }

    // ============ create progrms for each mesh/material variant combo ========================
    for (Renderable& rend : renderables)
    {
        Material* mat = rend.material;
        rend.mergedVariant = rend.instance->variantBits.getUint64() + mat->variantBits.getUint64();

        vk::PrimitiveTopology topo = VulkanAPI::VkUtil::topologyToVk(
            rend.instance->topology); // TODO : check that the topology is correct

        VulkanAPI::ShaderProgram* prog =
            manager.findVariant({shaderHash, rend.mergedVariant, static_cast<uint32_t>(topo)});

        if (!prog)
        {
            // create new program
            VulkanAPI::ShaderParser variantParser;
            std::vector<VulkanAPI::ProgramManager::CachedKey> hashes {
                {shaderHash,
                 VulkanAPI::Shader::Type::Vertex,
                 rend.instance->variantBits.getUint64(),
                 static_cast<uint32_t>(topo)},
                {shaderHash, VulkanAPI::Shader::Type::Fragment, mat->variantBits.getUint64()}};

            // create the variant shader program
            prog = manager.build(variantParser, hashes);
            assert(prog);

            // add variant definitions before compiling
            VulkanAPI::GlslCompiler::VariantMap matVariants =
                Material::createVariants(mat->variantBits);
            VulkanAPI::GlslCompiler::VariantMap meshVariants =
                Renderable::createVariants(rend.instance->variantBits);

            prog->addVariants(matVariants, VulkanAPI::Shader::Type::Fragment);
            prog->addVariants(meshVariants, VulkanAPI::Shader::Type::Vertex);

            if (!manager.compile(variantParser, prog))
            {
                return false;
            }
        }
        // keep reference to the shader program within the renderable and material for easier lookup
        // when drawing
        rend.program = prog;
        mat->program = prog;
    }

    return true;
}

void OERenderableManager::createMaterialDescriptors(Material* mat, const TextureGroup& group)
{
    assert(mat);
    assert(renderables[0].program);

    // get the general material layout and use the variant info to detrmine whether that material
    // should be added to the material descriptor set
    auto& materialBindings = renderables[0].program->getMaterialBindings();
    if (materialBindings.empty())
    {
        return;
    }

    auto& driver = engine.getVkDriver();
    auto& cbManager = driver.getCbManager();

    mat->materialSet = materialBindings[0].set;

    // create the descriptor layout binding info
    std::vector<vk::DescriptorSetLayoutBinding> setBindings;
    for (size_t i = 0; i < materialBindings.size(); ++i)
    {
        // we expect the texture group and bindings to sync together
        assert(materialBindings.size() == group.textures.size());

        auto& matBind = materialBindings[i];

        // only add the descriptor if the sampler is used
        if (group.textures[i])
        {
            setBindings.push_back(
                vk::DescriptorSetLayoutBinding {matBind.bind,
                                                vk::DescriptorType::eCombinedImageSampler,
                                                1,
                                                vk::ShaderStageFlagBits::eFragment,
                                                nullptr});
        }
    }

    mat->descrLayout = std::make_unique<vk::DescriptorSetLayout>();
    vk::DescriptorSetLayoutCreateInfo layoutInfo(
        {}, static_cast<uint32_t>(setBindings.size()), setBindings.data());
    VK_CHECK_RESULT(driver.getContext().device.createDescriptorSetLayout(
        &layoutInfo, nullptr, mat->descrLayout.get()));

    // create the descriptor set
    // TODO: this low-level vulkan stuff shouldn't be here.
    mat->descriptorSet = std::make_unique<vk::DescriptorSet>();

    vk::DescriptorSetAllocateInfo allocInfo {
        cbManager.getDescriptorPool(), 1, mat->descrLayout.get()};
    VK_CHECK_RESULT(
        driver.getContext().device.allocateDescriptorSets(&allocInfo, mat->descriptorSet.get()));

    // also update the pipeline layout with the descriptor layout
    mat->program->getPLineLayout()->addDescriptorLayout(mat->materialSet, *mat->descrLayout);
}

bool OERenderableManager::update()
{
    auto& driver = engine.getVkDriver();
    auto& cbManager = driver.getCbManager();

    // upload the textures if something has changed - this needs more thought - should we clear all
    // materials and create again or should each material have its own local 'dirty' flag and we
    // check this? What about newly added materials?
    if (materialDirty)
    {
        // create material shader variants if needed
        if (!updateVariants())
        {
            return false;
        }

        // upload textures if required
        for (TextureGroup& group : textures)
        {
            Material* material = findMaterial(group.matName);
            assert(material);

            if (!material->descriptorSet)
            {
                createMaterialDescriptors(material, group);
            }

            for (uint8_t i = 0; i < MaterialInstance::TextureType::Count; ++i)
            {
                MappedTexture* tex = group.textures[i];
                if (!tex)
                {
                    continue;
                }

                // each sampler needs its own unique id - so append the tex type to the
                // material name
                assert(!group.matName.empty());
                Util::String texShaderId =
                    Util::String::append(group.matName, TextureGroup::texTypeToStr(i));

                vk::ImageUsageFlagBits usageFlags = vk::ImageUsageFlagBits::eSampled;
                vk::Format format = VulkanAPI::VkUtil::imageFormatToVk(tex->getFormat());

                // note: we don't create a descriptor set alloc blueprint here as materials deal
                // with their own descriptor layouts
                MaterialInstance::Sampler& sampler = material->instance->sampler;
                VulkanAPI::Texture* vkTex = driver.findOrCreateTexture2d(
                    texShaderId,
                    format,
                    tex->getWidth(),
                    tex->getHeight(),
                    tex->getMipLevelCount(),
                    usageFlags);

                // add the material sampler
                vkTex->createSampler(
                    driver.getContext(),
                    VulkanAPI::Texture::toVkFilter(sampler.magFilter),
                    VulkanAPI::Texture::toVkFilter(sampler.minFilter),
                    VulkanAPI::Texture::toVkAddressMode(sampler.addressModeU),
                    VulkanAPI::Texture::toVkAddressMode(sampler.addressModeV),
                    8.0f); // TODO: user-defined max antriospy

                vkTex->map(driver, tex->getBuffer());

                // update the descriptor set too as we have the image info
                cbManager.updateTextureDescriptor(
                    i, vk::DescriptorType::eCombinedImageSampler, *material->descriptorSet, vkTex);
            }
        }
        materialDirty = false;
    }

    // upload meshes to the vulkan backend
    if (meshDirty)
    {
        // create a "mega" buffer from all the vertex and index data we have. Best pratice in
        // vulkan with regards to buffers is to create as few as possible - there is a upper
        // limit on the amount of allocations that can be performed
        updateBuffers();
        meshDirty = false;
    }

    return true;
}

Material* OERenderableManager::findMaterial(const Util::String& name)
{
    for (auto& mat : materials)
    {
        if (mat.instance->getName().compare(name))
        {
            return &mat;
        }
    }
    return nullptr;
}

} // namespace OmegaEngine
