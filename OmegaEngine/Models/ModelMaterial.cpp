#include "ModelMaterial.h"

namespace OmegaEngine
{

ModelMaterial::ModelMaterial()
{
}

ModelMaterial::~ModelMaterial()
{
}

Util::String ModelMaterial::getTextureUri(cgltf_texture_view& view)
{
	// not guaranteed to have a texture or uri
	if (view.texture && view.texture->image)
	{
		return view.texture->image->uri;
	}
	return "";
}

bool ModelMaterial::prepare(cgltf_material& mat)
{
	// two pipelines, either specular glosiness or metallic roughness
	// according to the spec, metallic roughness shoule be preferred
	if (mat.has_pbr_specular_glossiness)
	{
		usingSpecularGlossiness = true;

		textures.baseColour = getTextureUri(mat.pbr_specular_glossiness.diffuse_texture);
		factors.baseColour = OEMaths::vec4f(mat.pbr_specular_glossiness.diffuse_factor);
		factors.specularGlossiness = mat.pbr_specular_glossiness.glossiness_factor;
	}
	

	else if (mat.has_pbr_metallic_roughness)
	{
		usingSpecularGlossiness = false;

		textures.baseColour = getTextureUri(mat.pbr_metallic_roughness.base_color_texture);
		factors.baseColour = OEMaths::vec4f(mat.pbr_metallic_roughness.base_color_factor);
	}

	if (mat.normal_texture.texture)
	{
		textures.normal = getTextureUri(mat.normal_texture);
	}

	// specular
	
	blending.alphaMaskCutOff = mat.alpha_cutoff;
	blending.mask = Util::String(mat.alpha_mode);
}




}    // namespace OmegaEngine
