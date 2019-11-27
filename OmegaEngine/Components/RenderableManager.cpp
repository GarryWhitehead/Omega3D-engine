#include "RenderableManager.h"

#include "OEMaths/OEMaths_transform.h"

#include "Types/GpuResources.h"

#include "Core/Engine.h"

#include "VulkanAPI/Shader.h"
#include "VulkanAPI/VkDriver.h"

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

void RenderableManager::addMesh(MeshInstance& mesh, const size_t idx, const size_t offset)
{
	Renderable rend;

	// copy the meshes into the main buffer
	std::copy(mesh.vertices.begin(), mesh.vertices.end(), vertices.end());

	// and the indices
	std::copy(mesh.indices.begin(), mesh.indices.end(), indices.end());

	// straight copy for the primitives
	rend.primitives = mesh.primitives;

	// now adjust the material index values of each primitive to take into account
	// the offset
	if (offset >= 0)    //< a value of -1 indicate no materials for this renderable
	{
		for (auto& prim : rend.instance.primitives)
		{
			assert(prim.materialId >= 0);
			prim.materialId += offset;
		}
	}

	// check whether we just add to the back or use a freed slot
	if (renderables.size() < idx)
	{
		renderables.emplace_back(rend);
	}
	else
	{
		renderables[idx] = rend;
	}
}

void RenderableManager::addMesh(MeshInstance& mesh, Object& obj, const size_t offset)
{
	size_t idx = addObject(obj);
	addMesh(mesh, idx, offset);
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

size_t RenderableManager::addMaterial(MaterialInstance* mat, size_t count)
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
		materials.emplace_back(std::move(*mat));

		++mat;
	}

	return startOffset;
}

void RenderableManager::addRenderable(MeshInstance& mesh, MaterialInstance* mat, const size_t matCount, Object& obj)
{
	// first add the object which will give us a free slot
	size_t idx = addObject(obj);

	size_t matOffset = -1;

	// note: materials don't require a slot as there might be more than one material
	// per mesh. Instead the primitive stores a index to the required material
	if (mat)
	{
		matOffset = addMaterial(mat, matCount);
	}

	// now add the mesh and material and any textures
	addMesh(mesh, idx, matOffset);
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

	VulkanAPI::ShaderManager* manager = engine.getVkDriver().getShaderManager();

	// Note - we try and create as many shader variants as possible for vertex and material shaders as
	// creating them whilst the engine is actually rendering will be costly in terms of performance
	// create variants for all vertices associated with this manager
	for (const Renderable& rend : renderables)
	{

		if (!manager->hasShaderVariantCached(mesh_filename, rend.renderState, rend.variantBits.getUint64()))
		{
			VulkanAPI::ShaderDescriptor* descr =
			    manager->createCachedInstance(mesh_filename, rend.renderState, rend.variantBits.getUint64());

			mesh_parser.prepareShader(mesh_filename, descr, VulkanAPI::Shader::StageType::Vertex);
		}
	}

	// ======== create variants required for all materials currently associated with the manager =========

	// parse the shader json file - this will be used by all variants
	const Util::String mat_filename = "gbuffer_frag.glsl";
	VulkanAPI::ShaderParser mat_parser;

	if (!mat_parser.parse(mat_filename))
	{
		return false;
	}

	for (const Material& mat : materials)
	{
		if (!manager->hasShaderVariantCached(mat_filename, mat.renderState, mat.variantBits.getUint64()))
		{
			VulkanAPI::ShaderDescriptor* descr =
			    manager->createCachedInstance(mat_filename, rend.renderState, mat.variantBits.getUint64());

			mesh_parser.prepareShader(mat_filename, descr, VulkanAPI::Shader::StageType::Fragment);
		}
	}

	// now assemble into complete shader programs using the cached shader parts
	for (const Renderable& rend : renderables)
	{
		// each primitive can have its own material
		for (MeshInstance::Primitive& prim : rend.instance.primitives)
		{
			assert(prim.materialId >= 0);
			Material& mat = materials[prim.materialId];

			// first check that we don't already have a variant within the list
			// combine the material and mesh varaints to give us a unique id
			prim.mergedVariant = mat.variantBits.getUint64() + rend.variantBits.getUint64();

			if (manager->hasShaderVariant(mesh_filename, rend.renderState, prim.mergedVariant))
			{
				continue;
			}

			VulkanAPI::ShaderProgram* program = manager->createNewInstance(mesh_filename, rend.renderState, mergedVariant);

			// get the cached vertex stage and add to program
			VulkanAPI::ShaderDescriptor* descr =
			    manager->getCachedStage(mesh_filename, rend.renderBlockState, rend.variantBits);
			assert(descr);
			program->prepareStage(descr);

			// and get the cached fragment stage and add to program
			VulkanAPI::ShaderDescriptor* descr =
			    manager->getCachedStage(mesh_filename, rend.renderBlockState, rend.variantBits);
			assert(descr);
			program->prepareStage(descr);
		}
	}
}

void RenderableManager::update()
{
	VulkanAPI::VkDriver& driver = engine.getVkDriver();

	// upload the textures if something has changed
	if (materialDirty)
	{
		// create material shader varinats if needed
		updateVariants();

		// upload textures if required
		for (const TextureGroup& group : textures)
		{
			for (uint8_t i = 0; i < TextureType::Count; ++i)
			{
				if (group.textures[i])
				{
					MappedTexture* tex = group.textures[i]->texture;
					group.textures[i]->handle =
					    driver.add2DTexture(tex->format, tex->width, tex->height, tex->mipLevels, tex->data);
				}
			}
		}

		// upload ubos
		for (const Material& mat : materials)
		{
			driver->addUbo(mat.ubo);
		}
	}

	// upload meshes to the vulkan backend
	if (meshDirty)
	{
		for (const Renderable& rend : renderables)
		{
			driver.addVertexBuffer(rend.vertices.size, rend.vertices.data, rend.vertices.attributes);
			driver.addIndexBuffer(rend.indices.size(), rend.indices.data());
		}
	}
}

}    // namespace OmegaEngine
