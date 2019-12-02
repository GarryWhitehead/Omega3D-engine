#include "RenderableManager.h"

#include "OEMaths/OEMaths_transform.h"

#include "Types/GpuResources.h"

#include "Core/Engine.h"

#include "VulkanAPI/Shader.h"
#include "VulkanAPI/VkDriver.h"

#include "VulkanAPI/Compiler/ShaderParser.h"
#include "VulkanAPI/ProgramManager.h"

#include "Utility/GeneralUtil.h"
#include "Utility/logger.h"


namespace OmegaEngine
{

RenderableManager::RenderableManager(Engine& engine)
    : engine(engine)
{
	// for performance purposes
	renderables.reserve(MESH_INIT_CONTAINER_SIZE);
}

RenderableManager::~RenderableManager()
{
}

void RenderableManager::addMesh(Renderable& input, MeshInstance& mesh, const size_t idx, const size_t offset)
{
	// copy the vertices and indices into the "mega" buffer
    input.instance = mesh;
    
	// now adjust the material index - this is used primarily by the sorting key
	if (offset >= 0)    // a value of -1 indicate no materials for this renderable
	{
		// this is for debugging - we only allow one material per mesh so check this
		uint32_t mat_idx = 0;
		for (size_t i = 0; i < mesh.primitives.size(); ++i)
		{
			uint32_t newIdx = mesh.primitives[i].materialIdx;
			assert(idx != UINT32_MAX);
			if (i > 0 && newIdx != mat_idx)
			{
				// error here? just warn for now
				LOGGER_INFO("Warning: This mesh has more than one material for its primitives. Only one material per "
				            "mesh supported at present.");
			}
			mat_idx = newIdx;
		}
		input.materialId = idx + offset;
	}
}

void RenderableManager::addMesh(Renderable& input, MeshInstance& mesh, Object& obj, const size_t offset)
{
	size_t idx = addObject(obj);
	addMesh(input, mesh, idx, offset);
}

bool RenderableManager::prepareTexture(Util::String path, GpuTextureInfo* tex)
{
	if (!path.empty())
	{
		tex->texture = new MappedTexture;
		if (!tex->texture->load(path))
		{
			return false;
		}
	}
	// it's not an error if the path is empty.
	return true;
}

size_t RenderableManager::addMaterial(Renderable& input, MaterialInstance* mat, size_t count)
{
	assert(mat);
	assert(count > 0);

	size_t startOffset = materials.size();

	for (size_t i = 0; i < count; ++i)
	{
		// sort out the textures
		TextureGroup group;

		for (size_t j = 0; j < TextureType::Count; ++j)
		{
			// this function does return an error value but unsure yet
			// whether just to continue (it will be obvious that somethings gone
			// wrong when rendered) or return an error
			prepareTexture(mat->texturePaths[i], group.textures[j]);
		}

		textures.emplace_back(std::move(group));

		// and create the material - copy the data rather than keep the pointer to avoid issues later
		Material newMat;
		newMat.instance = *mat++;
		materials.emplace_back(newMat);
		input.material = &materials.back();
	}

	return startOffset;
}

void RenderableManager::addRenderable(MeshInstance& mesh, MaterialInstance* mat, const size_t matCount, Object& obj)
{
	Renderable newRend;

	// first add the object which will give us a free slot
	size_t idx = addObject(obj);

	// note: materials don't require a slot as there might be more than one material
	// per mesh. Instead the primitive stores a index to the required material
	size_t matOffset;
	if (mat)
	{
		matOffset = addMaterial(newRend, mat, matCount);
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

Renderable& RenderableManager::getMesh(Object& obj)
{
	size_t index = getObjIndex(obj);
	if (!index)
	{
		LOGGER_ERROR("Unable to find object within renderable manager.\n");
		return {};    // TODO: Better error handling here
	}
	return renderables[index];
}

void RenderableManager::updateBuffers()
{
    // calculate how much we need to allocate on the gpu side
    size_t vertTotalSize = 0;
    size_t indTotalSize = 0;
    
    for (Renderable& rend : renderables)
    {
        vertTotalSize += rend.vertexBuffer.size;
        indTotalSize += rend.indices.size();
    }
    
    char* vertData = new char[vertTotalSize];
    uint32_t* indData = new uint32_t[indTotalSize];
    
    // copy the data into the buffers
    char* vertPtr = vertData;
    uint32_t* indPtr = indData;
    
    for (Renderable& rend : renderables)
    {
        size_t vertSize = rend.vertexBuffer.size;
        size_t indSize = rend.indices.size() * sizeof(uint32_t);
        
        memcpy(vertPtr, rend.vertexBuffer.data, vertSize);
        memcpy(indPtr, rend.indices.data(), indSize);
        
        vertPtr += vertSize;
        indPtr += indSize;
    }
    
    // upload "mega" buffer to the gpu
    VulkanAPI::VkDriver& driver = engine.getVkDriver();
    
    // If no buffers exsisit, create new instances and upload data
    if (!vertexBuffer && !indexBuffer)
    {
        vertexBuffer = driver.addVertexBuffer(vertTotalSize, vertData, vertices.attributes);
        indexBuffer = driver.addIndexBuffer(indTotalSize, indData);
    }
    else if (vertexBuffer->getSize() < vertTotalSize)
    {
        // if the current buffer does not have enough space, destroy and create new buffer instances
        driver.destroyVertexBuffer(vertexBuffer);
        driver.destroyIndexBuffer(indexBuffer);

        vertexBuffer = driver.addVertexBuffer(vertTotalSize, vertData, vertices.attributes);
        indexBuffer = driver.addIndexBuffer(indTotalSize, indData);
    }
    else
    {
        vertexBuffer.map(vertTotalSize, vertData, vertices.attributes);
        indexBuffer.map(indTotalSize, indData);
    }
}

bool RenderableManager::updateVariants()
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

		if (!manager.hasShaderVariantCached({ mesh_filename.c_str(), rend.variantBits.getUint64(), rend.renderState }))
		{
			VulkanAPI::ShaderDescriptor* descr =
			    manager.createCachedInstance({ mesh_filename.c_str(), rend.variantBits.getUint64(), rend.renderState });

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
		if (!manager.hasShaderVariantCached({ mat_filename.c_str(),  mat.variantBits.getUint64(), nullptr }))
		{
			VulkanAPI::ShaderDescriptor* descr =
			    manager.createCachedInstance({ mat_filename.c_str(), mat.variantBits.getUint64(), nullptr });

			mesh_parser.prepareShader(mat_filename, descr, VulkanAPI::Shader::Type::Fragment);
		}
	}

	// ============ create progrms for each mesh/material variant combo ========================
	// We do this here for two reasons  1) Creating the shaders during time critical moments could really impede performance
	// 2) Its easier to update the material descriptor set as we have all the information available here.
	for (Renderable& rend : renderables)
	{
		Material* mat = rend.material;
		rend.mergedVariant = rend.variantBits.getUint64() + mat->variantBits.getUint64();

		VulkanAPI::ShaderProgram* prog = manager.findVariant({ mesh_filename.c_str(), rend.mergedVariant, rend.renderState });
		if (!prog)
		{
			// create new program
			std::vector<VulkanAPI::ProgramManager::ShaderHash> hashes
			{
				{ mesh_filename.c_str(), rend.renderState, rend.variantBits }, 
				{ mat_filename.c_str(), nullptr, mat->variantBits }
			};
			prog = manager.build(hashes);
		}

		// for each texture, we update the descriptor set with the appropiate image and sampler
        TextureGroup& texGroup = textures[mat->texIdx];
        Material* mat = rend.material;
        
		for (uint8_t i = 0; i < TextureType::Count; ++i)
		{
			if (texGroup.textures[i])
			{
                Util::String id = Util::String::Append(mat.name, texTypeToStr(i));
                prog->addDescrSetUpdateInfo(id, sampler, imageview, layout);
			}
		}
	}
}

void RenderableManager::update()
{
	VulkanAPI::VkDriver& driver = engine.getVkDriver();

	// upload the textures if something has changed
	if (materialDirty)
	{
		// upload textures if required
		for (const TextureGroup& group : textures)
		{
			for (uint8_t i = 0; i < TextureType::Count; ++i)
			{
				GpuTextureInfo* texGroup = group.textures[i];
				if (texGroup)
				{
					// each sampler needs its own unique id - so append the tex type to the material name
					Util::String id = group.matId.append(TextureGroup::texTypeToStr((TextureType)i));

					MappedTexture* tex = texGroup->texture;
					driver.add2DTexture(id, tex->format, tex->width, tex->height, tex->mipLevels);
					driver.update2DTexture(id, 0, tex->buffer);
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
        createBuffers();
        
	}
	
}
}    // namespace OmegaEngine

}    // namespace OmegaEngine
