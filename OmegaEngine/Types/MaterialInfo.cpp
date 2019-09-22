#include "MaterialInfo.h"

#include "Models/ModelMaterial.h"

#include "Core/World.h"

#include "Resource/ResourceManager.h"

#include "OEMaths/OEMaths_transform.h"

namespace OmegaEngine
{

MaterialInfo::MaterialInfo()
{
}

MaterialInfo::~MaterialInfo()
{
}

void MaterialInfo::buildTexture(ModelMaterial::TextureInfo& tex, ResourceManager& manager)
{
	if (!tex.path.empty())
	{
		tex.id = Util::generateTypeId(tex.path.c_str());
		manager.addResource(tex.path, tex.id);
	}
}

void MaterialInfo::build(ModelMaterial& mat, World& world)
{
	auto& resManager = world.getResourceManager();

	// deal with the images. To allow identification of the materials later, a hash will
	// be generated from the path names before passing to the resource manager
	buildTexture(mat.textures.baseColour, resManager);
	buildTexture(mat.textures.emissive, resManager);
	buildTexture(mat.textures.metallicRoughness, resManager);
	buildTexture(mat.textures.normal, resManager);
	buildTexture(mat.textures.occlusion, resManager);

}

}    // namespace OmegaEngine
