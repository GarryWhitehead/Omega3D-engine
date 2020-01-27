#include "RenderableManager.h"

#include "Components/TransformManager.h"
#include "Core/Engine.h"
#include "ModelImporter/MeshInstance.h"
#include "OEMaths/OEMaths_transform.h"
#include "VulkanAPI/Compiler/ShaderParser.h"
#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/Utility.h"
#include "VulkanAPI/VkDriver.h"
#include "utility/GeneralUtil.h"
#include "utility/Logger.h"


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
    if (offset >= 0) // a value of -1 indicate no materials for this renderable
    {
        input.materialId = idx + offset;
    }
}

void OERenderableManager::addMesh(
    Renderable& input, MeshInstance* mesh, OEObject& obj, const size_t offset)
{
    size_t idx = addObject(obj);
    addMesh(input, mesh, idx, offset);
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
    size_t startOffset = materials.size();

    // sort out the textures
    TextureGroup group;
    group.matName = mat->name;

    for (size_t j = 0; j < MaterialInstance::TextureType::Count; ++j)
    {
        // this function does return an error value but unsure yet
        // whether just to continue (it will be obvious that somethings gone
        // wrong when rendered) or return an error
        group.textures[j] = prepareTexture(mat->texturePaths[j]);
    }

    textures.emplace_back(std::move(group));

    Material newMat;
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
    size_t idx = addObject(obj);

    // note: materials don't require a slot as there might be more than one material
    // per mesh. Instead the primitive stores a index to the required material
    size_t matOffset = -1;
    if (mat)
    {
        matOffset = addMaterial(newRend, mat);
    }

    // now add the mesh and material and any textures
    addMesh(newRend, mesh, idx, matOffset);

    // check whether we just add to the back or use a freed slot
    if (renderables.size() < idx)
    {
        renderables.emplace_back(newRend);
    }
    else
    {
        renderables[idx] = std::move(newRend);
    }
}

Renderable& OERenderableManager::getMesh(const ObjHandle handle)
{
    assert(handle > 0 && handle < renderables.size());
    return renderables[handle];
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
    // parse the shader json file - this will be used by all variants
    const Util::String mesh_filename = "gbuffer_vert.glsl";
    VulkanAPI::ShaderParser mesh_parser;

    if (!mesh_parser.loadAndParse(mesh_filename))
    {
        LOGGER_ERROR(
            "Unable to parse mesh vertex shader stage: %s\n Failed with error: %s",
            mesh_filename.c_str(),
            mesh_parser.getErrorString().c_str());
        return false;
    }

    VulkanAPI::ProgramManager manager = engine.getVkDriver().getProgManager();

    // Note - we try and create as many shader variants as possible for vertex and material shaders
    // as creating them whilst the engine is actually rendering will be costly in terms of
    // performance create variants for all vertices associated with this manager
    for (const Renderable& rend : renderables)
    {
        vk::PrimitiveTopology topo = VulkanAPI::VkUtil::topologyToVk(rend.instance->topology);
        VulkanAPI::ProgramManager::ShaderHash hash {
            mesh_filename.c_str(), rend.instance->variantBits.getUint64(), &topo};
        if (!manager.hasShaderVariantCached(hash))
        {
            VulkanAPI::ShaderDescriptor* descr = manager.createCachedInstance(hash);

            if (!mesh_parser.loadAndParse(mesh_filename, descr, VulkanAPI::Shader::Type::Vertex))
            {
                printf(
                    "Fatal error parsing mesh shader: Failed with error: %s",
                    mesh_parser.getErrorString().c_str());
                return false;
            }
        }
    }

    // ======== create variants required for all materials currently associated with the manager
    // ======== parse the shader json file - this will be used by all variants
    const Util::String mat_filename = "gbuffer_frag.glsl";
    VulkanAPI::ShaderParser mat_parser;

    if (!mat_parser.loadAndParse(mat_filename))
    {
        printf(
            "Fatal error parsing material shader: Failed with error: %s",
            mat_parser.getErrorString().c_str());
        return false;
    }

    for (const Material& mat : materials)
    {
        VulkanAPI::ProgramManager::ShaderHash hash {
            mat_filename.c_str(), mat.variantBits.getUint64(), nullptr};
        if (!manager.hasShaderVariantCached(hash))
        {
            VulkanAPI::ShaderDescriptor* descr = manager.createCachedInstance(hash);

            if (!mesh_parser.loadAndParse(mat_filename, descr, VulkanAPI::Shader::Type::Fragment))
            {
                printf(
                    "Fatal error parsing mesh shader: Failed with error: %s",
                    mat_parser.getErrorString().c_str());
                return false;
            }
        }
    }

    // ============ create progrms for each mesh/material variant combo ========================
    // We do this here for two reasons  1) Creating the shaders during time critical moments could
    // really impede performance 2) Its easier to update the material descriptor set as we have all
    // the information available here.
    for (Renderable& rend : renderables)
    {
        Material* mat = rend.material;
        rend.mergedVariant = rend.instance->variantBits.getUint64() + mat->variantBits.getUint64();

        vk::PrimitiveTopology topo = VulkanAPI::VkUtil::topologyToVk(rend.instance->topology);
        VulkanAPI::ProgramManager::ShaderHash hash {
            mesh_filename.c_str(), rend.mergedVariant, &topo};
        VulkanAPI::ShaderProgram* prog = manager.findVariant(hash);
        if (!prog)
        {
            // create new program
            std::vector<VulkanAPI::ProgramManager::ShaderHash> hashes {
                {mesh_filename.c_str(), rend.instance->variantBits.getUint64(), &topo},
                {mat_filename.c_str(), mat->variantBits.getUint64(), nullptr}};
            prog = manager.build(hashes);
        }
        // keep reference to the shader program within the renderable for easier lookup when drawing
        rend.program = prog;
    }

    return true;
}

void OERenderableManager::update()
{
    VulkanAPI::VkDriver& driver = engine.getVkDriver();

    // upload the textures if something has changed
    if (materialDirty)
    {
        // upload textures if required
        for (TextureGroup& group : textures)
        {
            for (uint8_t i = 0; i < MaterialInstance::TextureType::Count; ++i)
            {
                MappedTexture* tex = group.textures[i];
                if (tex)
                {
                    // each sampler needs its own unique id - so append the tex type to the material
                    // name
                    assert(!group.matName.empty());
                    group.matName =
                        Util::String::append(group.matName, TextureGroup::texTypeToStr(i));

                    vk::ImageUsageFlagBits usageFlags = vk::ImageUsageFlagBits::eSampled;
                    vk::Format format = VulkanAPI::VkUtil::imageFormatToVk(tex->format);
                    driver.add2DTexture(
                        group.matName, format, tex->width, tex->height, tex->mipLevels, usageFlags);
                    driver.update2DTexture(group.matName, tex->buffer);
                }
            }
        }

        // create material shader varinats if needed
        updateVariants();
    }

    // upload meshes to the vulkan backend
    if (meshDirty)
    {
        // create a "mega" buffer from all the vertex and index data we have. Best pratice in vulkan
        // with regards to buffers is to create as few as possible - there is a upper limit on the
        // amount of allocations that can be performed
        updateBuffers();
    }
}

} // namespace OmegaEngine
