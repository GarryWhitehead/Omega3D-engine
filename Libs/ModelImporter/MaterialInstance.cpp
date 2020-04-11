#include "MaterialInstance.h"

#include "Formats/GltfModel.h"

namespace OmegaEngine
{

MaterialInstance::MaterialInstance()
{
}

MaterialInstance::~MaterialInstance()
{
}

MaterialInstance::Sampler::Filter MaterialInstance::getSamplerFilter(int filter)
{
    Sampler::Filter result;
    switch (filter)
    {
    case 9728:
        result = Sampler::Filter::Nearest;
        break;
    case 9729:
        result = Sampler::Filter::Linear;
        break;
    case 9984:
        result = Sampler::Filter::Nearest;
        break;
    case 9985:
        result = Sampler::Filter::Nearest;
        break;
    case 9986:
        result = Sampler::Filter::Linear;
        break;
    case 9987:
        result = Sampler::Filter::Linear;
        break;
    }
    return result;
}

MaterialInstance::Sampler::AddressMode MaterialInstance::getAddressMode(int mode)
{
    Sampler::AddressMode result;
    
    switch (mode)
    {
    case 10497:
        result = Sampler::AddressMode::Repeat;
        break;
    case 33071:
        result = Sampler::AddressMode::ClampToEdge;
        break;
    case 33648:
        result = Sampler::MirroredRepeat;
        break;
    }
    return result;
}

Util::String MaterialInstance::convertToAlpha(const cgltf_alpha_mode mode)
{
    Util::String result;
    switch(mode)
    {
        case cgltf_alpha_mode_opaque:
            result = "Opaque";
            break;
        case cgltf_alpha_mode_mask:
            result = "Mask";
            break;
        case cgltf_alpha_mode_blend:
            result = "Blend";
            break;
    }
    return result;
}

Util::String MaterialInstance::getTextureUri(cgltf_texture_view& view)
{
	// not guaranteed to have a texture or uri
	if (view.texture && view.texture->image)
	{
		// check whether this texture has a sampler. Otherwise use defeult values
        if (view.texture->sampler)
        {
            sampler.magFilter = getSamplerFilter(view.texture->sampler->mag_filter);
            sampler.minFilter = getSamplerFilter(view.texture->sampler->min_filter);
            sampler.addressModeU = getAddressMode(view.texture->sampler->wrap_s);
            sampler.addressModeV = getAddressMode(view.texture->sampler->wrap_t);
        }
        
        // also set variant bit
		return Util::String{ view.texture->image->uri };
	}
	return "";
}

bool MaterialInstance::prepare(cgltf_material& mat, GltfExtension& extensions)
{
    name = mat.name;
    
    // two pipelines, either specular glosiness or metallic roughness
	// according to the spec, metallic roughness shoule be preferred
	if (mat.has_pbr_specular_glossiness)
	{
		usingSpecularGlossiness = true;

        texturePaths[TextureType::BaseColour] = getTextureUri(mat.pbr_specular_glossiness.diffuse_texture);
		block.specularGlossiness = mat.pbr_specular_glossiness.glossiness_factor;
        
        auto* df = mat.pbr_specular_glossiness.diffuse_factor;
        block.baseColour = OEMaths::vec4f{df[0], df[1], df[2], df[3]};
	}
	

	else if (mat.has_pbr_metallic_roughness)
	{
		usingSpecularGlossiness = false;

        texturePaths[TextureType::BaseColour] = getTextureUri(mat.pbr_metallic_roughness.base_color_texture);
        texturePaths[TextureType::MetallicRoughness] = getTextureUri(mat.pbr_metallic_roughness.metallic_roughness_texture);
		block.roughness = mat.pbr_metallic_roughness.roughness_factor;
		block.metallic = mat.pbr_metallic_roughness.metallic_factor;
        
        auto* bcf = mat.pbr_metallic_roughness.base_color_factor;
        block.baseColour = OEMaths::vec4f{bcf[0], bcf[1], bcf[2], bcf[3]};
	}

	// normal texture
    texturePaths[TextureType::Normal] = getTextureUri(mat.normal_texture);

	// occlusion texture
    texturePaths[TextureType::Occlusion] = getTextureUri(mat.occlusion_texture);

	// emissive texture
    texturePaths[TextureType::Emissive] = getTextureUri(mat.emissive_texture);
	
	// emissive factor
    auto* ef = mat.emissive_factor;
    block.emissive = OEMaths::vec3f{ef[0], ef[1], ef[2]};

	// specular - extension
    Util::String data = extensions.find("specular");
    if (!data.empty())
	{
        block.specular = GltfExtension::tokenToVec3(data);
	}
	
	// alpha blending
	block.alphaMaskCutOff = mat.alpha_cutoff;
	block.mask = convertToAlpha(mat.alpha_mode);

	// determines the type of culling required
	doubleSided = mat.double_sided;
    
    return true;
}

bool MaterialInstance::prepare(aiMaterial* mat)
{
    // TODO!!

    /*mat->Get(AI_MATKEY_NAME, this->name.c_str());
    
    // factors
    aiColor4D ambient;
    mat->Get(AI_MATKEY_COLOR_AMBIENT, ambient);
    block.baseColour = OEMaths::vec4f(ambient.r);
    
    aiColor4D diffuse;
    mat->Get(AI_MATKEY_COLOR_AMBIENT, diffuse);
    block.diffuse = OEMaths::vec4f(diffuse.r);
    
    aiColor4D specular;
    mat->Get(AI_MATKEY_COLOR_SPECULAR, specular);
    block.specular = OEMaths::vec3f(specular.r);
    
    aiColor3D emissive;
    mat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive);
    block.specular = OEMaths::vec3f(emissive.r);
    
    // alpha
    
    // textures
    aiString diffusePath;
    mat->GetTexture(aiTextureType_DIFFUSE, 0, &diffusePath);
    texturePaths[TextureType::BaseColour] = diffusePath.C_Str();
    
    aiString normalPath;
    mat->GetTexture(aiTextureType_NORMALS, 0, &normalPath);
    texturePaths[TextureType::Normal] = normalPath.C_Str();
    
    aiString emissivePath;
    mat->GetTexture(aiTextureType_EMISSIVE, 0, &emissivePath);
    texturePaths[TextureType::Emissive] = emissivePath.C_Str();
    */
    return true;
}

}    // namespace OmegaEngine
