#include "RenderableManager.h"

#include "OEMaths/OEMaths_transform.h"

#include "Core/Engine.h"

#include "Components/TransformManager.h"

#include "ModelImporter/MaterialInstance.h"
#include "ModelImporter/MeshInstance.h"

#include "VulkanAPI/Shader.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/Descriptors.h"

#include "VulkanAPI/Compiler/ShaderParser.h"
#include "VulkanAPI/ProgramManager.h"

#include "utility/GeneralUtil.h"
#include "utility/Logger.h"


namespace OmegaEngine
{

// =================================================================================
// User interface renderable building functions

RenderableManager::Instance& RenderableManager::Instance::addMesh(MeshInstance* instance)
{
    assert(instance);
    mesh = instance;
    return *this;
}

RenderableManager::Instance& RenderableManager::Instance::addMaterial(MaterialInstance* instance)
{
    assert(instance);
    mat = instance;
    return *this;
}

RenderableManager::Instance& RenderableManager::Instance::addSkin(SkinInstance* instance)
{
    assert(instance);
    skin = instance;
    return *this;
}

RenderableManager::Instance& RenderableManager::Instance::addNode(NodeInstance* instance)
{
    assert(instance);
    node = instance;
    return *this;
}

void RenderableManager::Instance::create(Engine& engine, Object* obj)
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
        LOGGER_WARN("You have called create but this instance has no mesh or transform data associated with it.");
        return;
    }
    
    OERenderableManager& rManager = oeEngine.getRendManager();
    TransformManager& tManager = oeEngine.getTransManager();
    
    // we don't have to check whether the material or skin in init, the managers will do that
    rManager.addRenderable(mesh, mat, *obj);
    tManager.addNodeHierachy(*node, *obj, skin);
}

// =================================================================================
Util::String TextureGroup::texTypeToStr(const int type)
{
    assert(type < static_cast<int>(TextureType::Count));
    Util::String result;
	switch (type)
	{
	case TextureType::BaseColour:
		result = "BaseColour";
		break;
	case TextureType::Emissive:
		result = "Emissive";
		break;
	case TextureType::MetallicRoughness:
		result = "MetallicRoughness";
		break;
	case TextureType::Normal:
		result = "Normal";
		break;
	case TextureType::Occlusion:
		result = "Occlusion";
		break;
	}
	return result;
}

// ==============================================================================

Material::~Material()
{
   if (instance)
   {
       delete instance;
       instance = nullptr;
   }
}

// ===============================================================================================

OERenderableManager::OERenderableManager(OEEngine& engine)
    : engine(engine)
{
	// for performance purposes
	renderables.reserve(MESH_INIT_CONTAINER_SIZE);
}

OERenderableManager::~OERenderableManager()
{
}

void OERenderableManager::addMesh(Renderable& input, MeshInstance* mesh, const size_t idx, const size_t offset)
{
	// copy the vertices and indices into the "mega" buffer
	input.instance = mesh;

	// now adjust the material index - this is used primarily by the sorting key
	if (offset >= 0)    // a value of -1 indicate no materials for this renderable
	{
		input.materialId = idx + offset;
	}
}

void OERenderableManager::addMesh(Renderable& input, MeshInstance* mesh, Object& obj, const size_t offset)
{
	size_t idx = addObject(obj);
	addMesh(input, mesh, idx, offset);
}

bool OERenderableManager::prepareTexture(Util::String path, MappedTexture* tex)
{
	if (!path.empty())
	{
		tex = new MappedTexture();
		if (!tex->load(path))
		{
			return false;
		}
	}
	// it's not an error if the path is empty.
	return true;
}

size_t OERenderableManager::addMaterial(Renderable& input, MaterialInstance* mat)
{
	size_t startOffset = materials.size();

    // sort out the textures
    TextureGroup group;

    for (size_t j = 0; j < TextureGroup::TextureType::Count; ++j)
    {
        // this function does return an error value but unsure yet
        // whether just to continue (it will be obvious that somethings gone
        // wrong when rendered) or return an error
        prepareTexture(mat->texturePaths[j], group.textures[j]);
    }

    textures.emplace_back(std::move(group));

    Material newMat;
    newMat.instance = mat;
    materials.emplace_back(std::move(newMat));
    input.material = &materials.back();
	
	return startOffset;
}

void OERenderableManager::addRenderable(MeshInstance* mesh, MaterialInstance* mat, Object& obj)
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

	if (!mesh_parser.parse(mesh_filename))
	{
		LOGGER_ERROR("Unable to parse mesh vertex shader stage: %s", mesh_filename.c_str());
		return false;
	}

	VulkanAPI::ProgramManager manager = engine.getVkDriver().getProgManager();

	// Note - we try and create as many shader variants as possible for vertex and material shaders as
	// creating them whilst the engine is actually rendering will be costly in terms of performance
	// create variants for all vertices associated with this manager
	for (const Renderable& rend : renderables)
	{
        VulkanAPI::ProgramManager::ShaderHash hash { mesh_filename.c_str(), rend.instance->variantBits.getUint64(), &rend.instance->topology };
		if (!manager.hasShaderVariantCached(hash))
		{
			VulkanAPI::ShaderDescriptor* descr =
			    manager.createCachedInstance(hash);

			mesh_parser.prepareShader(mesh_filename, descr, VulkanAPI::Shader::Type::Vertex);
		}
	}

	// ======== create variants required for all materials currently associated with the manager ========
	// parse the shader json file - this will be used by all variants
	const Util::String mat_filename = "gbuffer_frag.glsl";
	VulkanAPI::ShaderParser mat_parser;

	if (!mat_parser.parse(mat_filename))
	{
		return false;
	}

	for (const Material& mat : materials)
	{
        VulkanAPI::ProgramManager::ShaderHash hash{mat_filename.c_str(), mat.variantBits.getUint64(), nullptr};
        if (!manager.hasShaderVariantCached(hash))
		{
			VulkanAPI::ShaderDescriptor* descr =
			    manager.createCachedInstance(hash);

			mesh_parser.prepareShader(mat_filename, descr, VulkanAPI::Shader::Type::Fragment);
		}
	}

	// ============ create progrms for each mesh/material variant combo ========================
	// We do this here for two reasons  1) Creating the shaders during time critical moments could really impede performance
	// 2) Its easier to update the material descriptor set as we have all the information available here.
	for (Renderable& rend : renderables)
	{
		Material* mat = rend.material;
		rend.mergedVariant = rend.instance->variantBits.getUint64() + mat->variantBits.getUint64();
        
        VulkanAPI::ProgramManager::ShaderHash hash { mesh_filename.c_str(), rend.mergedVariant, &rend.instance->topology };
		VulkanAPI::ShaderProgram* prog =
		    manager.findVariant(hash);
		if (!prog)
		{
			// create new program
			std::vector<VulkanAPI::ProgramManager::ShaderHash> hashes{
				{ mesh_filename.c_str(), rend.instance->variantBits.getUint64(), &rend.instance->topology },
				{ mat_filename.c_str(), mat->variantBits.getUint64(), nullptr }
			};
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
		for (const TextureGroup& group : textures)
		{
			for (uint8_t i = 0; i < TextureGroup::TextureType::Count; ++i)
			{
				MappedTexture* tex = group.textures[i];
				if (tex)
				{
					// each sampler needs its own unique id - so append the tex type to the material name
					Util::String id = Util::String::append(group.matName, TextureGroup::texTypeToStr(i));

                    vk::ImageUsageFlagBits usageFlags = vk::ImageUsageFlagBits::eSampled;
					driver.add2DTexture(id, tex->format, tex->width, tex->height, tex->mipLevels, usageFlags);
					driver.update2DTexture(id, tex->buffer);
				}
			}
		}

		// create material shader varinats if needed
		updateVariants();
	}

	// upload meshes to the vulkan backend
	if (meshDirty)
	{
		// create a "mega" buffer from all the vertex and index data we have. Best pratice in vulkan with regards to buffers
		// is to create as few as possible - there is a upper limit on the amount of allocations that can be performed
		updateBuffers();
	}
}

}    // namespace OmegaEngine
