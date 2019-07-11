#include "ModelMaterial.h"

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
		textureOutput = textures.baseColour;
		break;
	case TextureId::Emissive:
		textureOutput = textures.emissive;
		break;
	case TextureId::MetallicRoughness:
		textureOutput = textures.metallicRoughness;
		break;
	case TextureId::Normal:
		textureOutput = textures.normal;
		break;
	case TextureId::Occlusion:
		textureOutput = textures.occlusion;
		break;
	}

	return textureOutput;
}


} // namespace OmegaEngine
