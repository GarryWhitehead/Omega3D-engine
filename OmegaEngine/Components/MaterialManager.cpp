#include "MaterialManager.h"

#include "Core/World.h"

#include "Models/ModelMaterial.h"

#include "Types/Texture.h"

namespace OmegaEngine
{

MaterialManager::MaterialManager()
{
}

MaterialManager::~MaterialManager()
{
}

bool MaterialManager::prepareTexture(Util::String path, Texture* tex)
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

bool MaterialManager::addMaterial(ModelMaterial& mat, World& world)
{
	// sort out the textures
	TextureGroup group;

	for (size_t i = 0; i < TextureType::Count; ++i)
	{
		if (!prepareTexture(mat.texturePaths[i], group.textures[i]))
		{
			return false;
		}
	}

	textures.emplace_back(std::move(group));

	// store the index of the textures for linking later
	mat.bufferIndex = materials.size();

	// destructive
	materials.emplace_back(std::move(mat));

}

ModelMaterial& MaterialManager::getMaterial(Object& obj)
{
	size_t index = objIndices[obj];
	assert(index < materials.size());
	return materials[index];
}

}    // namespace OmegaEngine