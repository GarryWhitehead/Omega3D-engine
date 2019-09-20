#include "ModelMaterial.h"

#include "Models/Formats/GltfModel.h"

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

bool ModelMaterial::prepare(cgltf_material& mat, ExtensionData& extensions)
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
		textures.metallicRoughness = getTextureUri(mat.pbr_metallic_roughness.metallic_roughness_texture);
		factors.baseColour = OEMaths::vec4f(mat.pbr_metallic_roughness.base_color_factor);
		factors.roughness = mat.pbr_metallic_roughness.roughness_factor;
		factors.metallic = mat.pbr_metallic_roughness.metallic_factor;
	}

	// normal texture
	textures.normal = getTextureUri(mat.normal_texture);

	// occlusion texture
	textures.occlusion = getTextureUri(mat.occlusion_texture);

	// emissive texture
	textures.emissive = getTextureUri(mat.emissive_texture);
	
	// emissive factor
	factors.emissive = OEMaths::vec3f(mat.emissive_factor);

	// specular - extension
	auto iter = extensions.find("specular");
	if (iter != extensions.end)
	{
		factors.specular = GltfModel::tokenToVec3(iter->second);
	}
	
	// alpha blending
	blending.alphaMaskCutOff = mat.alpha_cutoff;
	blending.mask = convertToAlpha(mat.alpha_mode);
}

bool ModelMaterial::prepare(aiMaterial* mat)
{
    mat->Get(AI_MATKEY_NAME, this->name);
    
    // factors
    aiColor4D ambient;
    mat->Get(AI_MATKEY_COLOR_AMBIENT, ambient);
    factors.baseColour = OEMaths::vec4f(ambient.r);
    
    aiColor4D diffuse;
    mat->Get(AI_MATKEY_COLOR_AMBIENT, diffuse);
    factors.diffuse = OEMaths::vec4f(diffuse.r);
    
    aiColor4D specular;
    mat->Get(AI_MATKEY_COLOR_SPECULAR, specular);
    factors.specular = OEMaths::vec3f(specular.r);
    
    aiColor3D emissive;
    mat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive);
    factors.specular = OEMaths::vec3f(emissive.r);
    
    // alpha
    
    // textures
    aiString diffusePath;
    mat->GetTexture(aiTextureType_DIFFUSE, 0, &diffusePath);
    textures.baseColour = diffusePath.C_Str();
    
    aiString normalPath;
    mat->GetTexture(aiTextureType_NORMALS, 0, &normalPath);
    textures.baseColour = normalPath.C_Str();
    
    aiString emissivePath;
    mat->GetTexture(aiTextureType_EMISSIVE, 0, &emissivePath);
    textures.emissive = emissivePath.C_Str();
    
    
}


}    // namespace OmegaEngine
