#include "Material.h"

#include "Core/Omega_Global.h"

#include "OEMaths/OEMaths_transform.h"

namespace OmegaEngine
{

Material::Material()
{
}

Material::~Material()
{
}

void Material::prepare(MaterialComponent* component)
{
	name = component->name;

	// important that the name is valid as this is used to trace textures in the vulkan backend
	assert(!name.empty() || name != "");

	factors.baseColour = component->baseColour;
	factors.diffuse = OEMaths::vec4f(component->diffuse, 1.0f);
	factors.emissive = component->emissive;
	factors.metallic = component->metallic;
	factors.roughness = component->roughness;
	factors.specular = component->specular;

	// only opaque supported for user-defined materials at the moment
	alphaMask = AlphaMode::Opaque;
}

void Material::prepare(ModelMaterial& material)
{
	name = material.name;

	// important that the name is valid as this is used to trace textures in the vulkan backend
	assert(!name.empty() || name != "");

	// like for like copy for the factors
	factors.baseColour = material.factors.baseColour;
	factors.diffuse = material.factors.diffuse;
	factors.emissive = material.factors.emissive;
	factors.metallic = material.factors.metallic;
	factors.roughness = material.factors.roughness;
	factors.specular = material.factors.specular;
	factors.specularGlossiness = material.factors.specularGlossiness;

	// uv sets - straight copy
	uvSets.baseColour = material.uvSets.baseColour;
	uvSets.diffuse = material.uvSets.diffuse;
	uvSets.emissive = material.uvSets.emissive;
	uvSets.metallicRoughness = material.uvSets.metallicRoughness;
	uvSets.normal = material.uvSets.normal;
	uvSets.occlusion = material.uvSets.occlusion;
	uvSets.specularGlossiness = material.uvSets.specularGlossiness;

	// blending
	std::string alphaMaskStr = material.factors.mask;
	if (alphaMaskStr == "BLEND")
	{
		alphaMask = AlphaMode::Blend;
	}
	else if (alphaMaskStr == "MASK")
	{
		alphaMask = AlphaMode::Mask;
	}
	else
	{
		alphaMask = AlphaMode::Opaque;
	}

	alphaMaskCutOff = material.factors.alphaMaskCutOff;

	usingSpecularGlossiness = material.usingSpecularGlossiness;
}

}    // namespace OmegaEngine
