#include "RenderableManager.h"

#include "OEMaths/OEMaths_transform.h"

#include "Types/Texture.h"

#include "Utility/GeneralUtil.h"
#include "Utility/logger.h"


namespace OmegaEngine
{

RenderableManager::RenderableManager()
{
	// for performance purposes
	renderables.reserve(MESH_INIT_CONTAINER_SIZE);
	vertices.reserve(1000000);    // these numbers need evaluating
	indices.reserve(1000000);
}

RenderableManager::~RenderableManager()
{
}

void RenderableManager::addMesh(ModelMesh& mesh, const size_t idx, const size_t offset)
{
	RenderableInfo rend;

	rend.vertOffset = vertices.size();
	rend.idxOffset = indices.size();

	// copy the meshes into the main buffer
	std::copy(mesh.vertices.begin(), mesh.vertices.end(), vertices.end());

	// and the indices
	std::copy(mesh.indices.begin(), mesh.indices.end(), indices.end());

	// straight copy for the primitives
	rend.primitives = mesh.primitives;

	// now adjust the material index values of each primitive to take into account
	// the offset
	if (offset >= 0)	//< a value of -1 indicate no materials for this renderable
	{
		for (auto& prim : rend.primitives)
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

void RenderableManager::addMesh(ModelMesh& mesh, Object& obj, const size_t offset)
{
	size_t idx = addObject(obj);
	addMesh(mesh, idx, offset);
}

bool RenderableManager::prepareTexture(Util::String path, Texture* tex)
{
	if (!path.empty())
	{
		tex = new Texture;
		if (!tex->load(path))
		{
			return false;
		}
	}
	// it's not an error if the path is empty.
	return true;
}

size_t RenderableManager::addMaterial(ModelMaterial* mat, size_t count)
{
	assert(mat);
	assert(count > 0);

	size_t startOffset = materials.size();

	for (size_t i = 0; i < count; ++i)
	{
		// sort out the textures
		TextureGroup group;

		for (size_t i = 0; i < TextureType::Count; ++i)
		{
			// this function does return an error value but unsure yet
			// whether just to continue (it will be obvious that somethings gone
			// wrong when rendered) or return an error
			prepareTexture(mat->texturePaths[i], group.textures[i]);
		}

		textures.emplace_back(std::move(group));
		materials.emplace_back(std::move(*mat));

		++mat;
	}

	return startOffset;
}

void RenderableManager::addRenderable(ModelMesh& mesh, ModelMaterial* mat, const size_t matCount, Object& obj)
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

RenderableManager::RenderableInfo& RenderableManager::getMesh(Object& obj)
{
	size_t index = getObjIndex(obj);
	if (!index)
	{
		LOGGER_ERROR("Unable to find object within renderable manager.\n");
		return {};    // TODO: Better error handling here
	}
	return renderables[index];
}

void RenderableManager::updateFrame()
{
	if (isDirty)
	{
		
		isDirty = false;
	}
}

}    // namespace OmegaEngine