#include "ModelMaterial.h"
#include "Utility/Logger.h"

#include <memory>

namespace OmegaEngine
{

ModelMaterial::ModelMaterial()
{
}

ModelMaterial::~ModelMaterial()
{
}

int32_t ModelMaterial::getTexture(const TextureId id)
{
	int32_t textureOutput = -1;

	switch (id)
	{
	case TextureId::BaseColour:
		textureOutput = material.textures.baseColour;
		break;
	case TextureId::Emissive:
		textureOutput = material.textures.emissive;
		break;
	case TextureId::MetallicRoughness:
		textureOutput = material.textures.metallicRoughness;
		break;
	case TextureId::Normal:
		textureOutput = material.textures.normal;
		break;
	case TextureId::Occlusion:
		textureOutput = material.textures.occlusion;
		break;
	}

	return textureOutput;
}


} // namespace OmegaEngine
