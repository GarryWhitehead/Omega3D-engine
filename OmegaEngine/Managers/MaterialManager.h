#pragma once

#include "Models/ModelMaterial.h"

#include "Types/Object.h"

#include <vector>

namespace OmegaEngine
{

// forward declerations
class World;
class ModelMaterial;
class Texture;

class MaterialManager
{
public:

	/**
	* @brief A convienent way to group textures together
	* supports PBR materials - if null, field not used
	*/
	struct TextureGroup
	{
		TextureGroup() = default;

		~TextureGroup()
		{
			for (size_t i = 0; i < TextureType::Count; ++i)
			{
				if (textures[i])
				{
					delete textures[i];
					textures[i] = nullptr;
				}
			}
		}

		// ensure not copyable
		TextureGroup(const TextureGroup&) = delete;
		TextureGroup& operator=(const TextureGroup&) = delete;

		// but moveable
		TextureGroup(TextureGroup&&) = default;
		TextureGroup& operator=(TextureGroup&&) = default;

		Texture* textures[TextureType::Count];
	};

	MaterialManager();
	~MaterialManager();

	// not copyable or moveable
	MaterialManager(const MaterialManager&) = delete;
	MaterialManager& operator=(const MaterialManager&) = delete;
	MaterialManager(MaterialManager&&) = delete;
	MaterialManager& operator=(MaterialManager&&) = delete;

	bool addMaterial(ModelMaterial& mat, World& world);

	ModelMaterial& getMaterial(Object& obj);

private:
	
	bool prepareTexture(Util::String path, Texture* tex);

private:

	// indice into the transform buffer for each object stored in this manager
	std::unordered_map<Object, size_t, ObjHash, ObjEqual> objIndices;

	// all the materials 
	std::vector<ModelMaterial> materials;

	// and all the textures
	std::vector<TextureGroup> textures;

};
}    // namespace OmegaEngine
