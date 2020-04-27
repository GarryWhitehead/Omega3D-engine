/* Copyright (c) 2018-2020 Garry Whitehead
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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

float MaterialInstance::convertToAlpha(const cgltf_alpha_mode mode)
{
    float result;
    switch(mode)
    {
        case cgltf_alpha_mode_opaque:
            result = 0.0f;
            break;
        case cgltf_alpha_mode_mask:
            result = 1.0f;
            break;
        case cgltf_alpha_mode_blend:
            result = 2.0f;
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
        
        // instead of having a seperate entry for mr or specular gloss, the two share the same slot. Should maybe be renamed to be more transparent?
        texturePaths[TextureType::MetallicRoughness] = getTextureUri(mat.pbr_specular_glossiness.specular_glossiness_texture);
        
		//block.specularGlossiness = mat.pbr_specular_glossiness.glossiness_factor;
        
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
    block.emissive = OEMaths::vec4f{ef[0], ef[1], ef[2], 1.0f};

	// specular - extension
    Util::String data = extensions.find("specular");
    if (!data.empty())
	{
        block.specular = OEMaths::vec4f {GltfExtension::tokenToVec3(data), 1.0f};
	}
	
	// alpha blending
	block.alphaMaskCutOff = mat.alpha_cutoff;
    block.alphaMask = convertToAlpha(mat.alpha_mode);

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
