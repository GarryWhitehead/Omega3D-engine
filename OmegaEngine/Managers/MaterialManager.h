#pragma once

#include "Types/MaterialInfo.h"
#include "Types/MappedTexture.h"

namespace OmegaEngine
{

class MaterialManager
{
public:

	MaterialManager();
	~MaterialManager();

	// not copyable or moveable
	MaterialManager(const MaterialManager&) = delete;
	MaterialManager& operator=(const MaterialManager&) = delete;
	MaterialManager(MaterialManager&&) = delete;
	MaterialManager& operator=(MaterialManager&&) = delete;

private:

	// all the materials 
	std::vector<MaterialInfo> materials;

	// and all the textures
	std::vector<Texture> textures;

};
}    // namespace OmegaEngine
