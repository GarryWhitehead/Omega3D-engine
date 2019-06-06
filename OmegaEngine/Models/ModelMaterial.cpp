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

	vk::SamplerAddressMode ModelMaterial::getWrapMode(int32_t wrap)
	{
		vk::SamplerAddressMode ret;

		switch (wrap)
		{
		case 10497:
			ret = vk::SamplerAddressMode::eRepeat;
			break;
		case 33071:
			ret = vk::SamplerAddressMode::eClampToEdge;
			break;
		case 33648:
			ret = vk::SamplerAddressMode::eMirroredRepeat;
			break;
		default:
			LOGGER_INFO("Unsupported wrap mode %i whilst parsing gltf sampler.", wrap);
			ret = vk::SamplerAddressMode::eClampToBorder;
		}
		return ret;
	}

	vk::Filter ModelMaterial::getFilterMode(int32_t filter)
	{
		vk::Filter ret;

		switch (filter)
		{
		case 9728:
			ret = vk::Filter::eNearest;
			break;
		case 9729:
			ret = vk::Filter::eLinear;
			break;
		case 9984:
			ret = vk::Filter::eNearest;
			break;
		case 9986:
			ret = vk::Filter::eLinear;
			break;
		case 9987:
			ret = vk::Filter::eLinear;
			break;
		default:
			LOGGER_INFO("Unsupported filter mode %i whilst parsing gltf sampler.", filter);
			ret = vk::Filter::eNearest;
		}
		return ret;
	}

	void ModelMaterial::extractfImageData(tinygltf::Model& model)
	{
		for (auto& tex : model.textures)
		{
			tinygltf::Image image = model.images[tex.source];

			auto& texture = std::make_unique<Texture>(image.name);

			// map to temporary storage until transferred to the asset manager
			texture->map(image.width, image.height, image.image.data());

			// nor guarenteed to have a sampler
			if (tex.sampler > -1)
			{
				tinygltf::Sampler gltfSampler = model.samplers[tex.sampler];

				vk::SamplerAddressMode mode = getWrapMode(gltfSampler.wrapS);
				vk::Filter filter = getFilterMode(gltfSampler.minFilter);

				texture->sampler = std::make_unique<Sampler>(mode, filter);
			}

			textures.emplace_back(std::move(texture));
		}
	}

	void ModelMaterial::extractMaterialData(tinygltf::Model& model, tinygltf::Material& gltfMaterial)
	{
		material.name = gltfMaterial.name;

		// go through each material type and see if they exsist - we are only saving the index
		if (gltfMaterial.values.find("baseColorTexture") != gltfMaterial.values.end())
		{
			material.textures.baseColour = gltfMaterial.values["baseColorTexture"].TextureIndex();
			material.uvSets.baseColour = gltfMaterial.values["baseColorTexture"].TextureTexCoord();
		}
		if (gltfMaterial.values.find("metallicRoughnessTexture") != gltfMaterial.values.end())
		{
			material.textures.metallicRoughness = gltfMaterial.values["metallicRoughnessTexture"].TextureIndex();
			material.uvSets.metallicRoughness = gltfMaterial.values["metallicRoughnessTexture"].TextureTexCoord();
		}
		if (gltfMaterial.values.find("baseColorFactor") != gltfMaterial.values.end())
		{
			material.factors.baseColour = OEMaths::vec4f(gltfMaterial.values["baseColorFactor"].ColorFactor().data());
		}
		if (gltfMaterial.values.find("metallicFactor") != gltfMaterial.values.end())
		{
			material.factors.metallic = static_cast<float>(gltfMaterial.values["metallicFactor"].Factor());
		}
		if (gltfMaterial.values.find("roughnessFactor") != gltfMaterial.values.end())
		{
			material.factors.roughness = static_cast<float>(gltfMaterial.values["roughnessFactor"].Factor());
		}

		// any additional textures?
		if (gltfMaterial.additionalValues.find("normalTexture") != gltfMaterial.additionalValues.end())
		{
			material.textures.normal = gltfMaterial.additionalValues["normalTexture"].TextureIndex();
			material.uvSets.normal = gltfMaterial.additionalValues["normalTexture"].TextureTexCoord();
		}
		if (gltfMaterial.additionalValues.find("emissiveTexture") != gltfMaterial.additionalValues.end())
		{
			material.textures.emissive = gltfMaterial.additionalValues["emissiveTexture"].TextureIndex();
			material.uvSets.emissive = gltfMaterial.additionalValues["emissiveTexture"].TextureTexCoord();
		}
		if (gltfMaterial.additionalValues.find("occlusionTexture") != gltfMaterial.additionalValues.end())
		{
			material.textures.occlusion = gltfMaterial.additionalValues["occlusionTexture"].TextureIndex();
			material.uvSets.occlusion = gltfMaterial.additionalValues["occlusionTexture"].TextureTexCoord();
		}

		// check for aplha modes
		if (gltfMaterial.additionalValues.find("alphaMode") != gltfMaterial.additionalValues.end())
		{
			tinygltf::Parameter param = gltfMaterial.additionalValues["alphaMode"];
			if (param.string_value == "BLEND")
			{
				material.factors.alphaMask = Material::AlphaMode::Blend;
			}
			if (param.string_value == "MASK")
			{
				material.factors.alphaMask = Material::AlphaMode::Mask;
			}
		}
		if (gltfMaterial.additionalValues.find("alphaCutOff") != gltfMaterial.additionalValues.end())
		{
			material.factors.alphaMaskCutOff = static_cast<float>(gltfMaterial.additionalValues["alphaCutOff"].Factor());
		}
		if (gltfMaterial.additionalValues.find("emissiveFactor") != gltfMaterial.additionalValues.end())
		{
			material.factors.emissive = OEMaths::vec3f(gltfMaterial.additionalValues["emissiveFactor"].ColorFactor().data());
		}

		// check for extensions
		auto extension = gltfMaterial.extensions.find("KHR_materials_pbrSpecularGlossiness");
		if (extension != gltfMaterial.extensions.end())
		{
			if (extension->second.Has("specularGlossinessTexture"))
			{
				auto index = extension->second.Get("specularGlossinessTexture").Get("index");
				material.textures.metallicRoughness = index.Get<int>();
				material.usingSpecularGlossiness = true;

				auto uv_index = extension->second.Get("specularGlossinessTexture").Get("texCoord");
				material.uvSets.specularGlossiness = uv_index.Get<int>();
			}
			if (extension->second.Has("diffuseTexture"))
			{
				auto index = extension->second.Get("diffuseTexture").Get("index");
				material.textures.baseColour = index.Get<int>();
				material.usingSpecularGlossiness = true;

				auto uvIndex = extension->second.Get("diffuseTexture").Get("texCoord");
				material.uvSets.diffuse = uvIndex.Get<int>();
			}
			if (extension->second.Has("diffuseFactor"))
			{
				auto factor = extension->second.Get("diffuseFactor");
				auto value = factor.Get(0);
				float x = value.IsNumber() ? (float)value.Get<double>() : (float)value.Get<int>();
				value = factor.Get(1);
				float y = value.IsNumber() ? (float)value.Get<double>() : (float)value.Get<int>();
				value = factor.Get(2);
				float z = value.IsNumber() ? (float)value.Get<double>() : (float)value.Get<int>();
				value = factor.Get(3);
				float w = value.IsNumber() ? (float)value.Get<double>() : (float)value.Get<int>();

				material.factors.diffuse = OEMaths::vec4f(x, y, z, w);
				material.usingSpecularGlossiness = true;
			}
			if (extension->second.Has("specularFactor"))
			{
				auto factor = extension->second.Get("specularFactor");
				auto value = factor.Get(0);
				float x = value.IsNumber() ? (float)value.Get<double>() : (float)value.Get<int>();
				value = factor.Get(1);
				float y = value.IsNumber() ? (float)value.Get<double>() : (float)value.Get<int>();
				value = factor.Get(2);
				float z = value.IsNumber() ? (float)value.Get<double>() : (float)value.Get<int>();

				material.factors.specular = OEMaths::vec3f(x, y, z);
				material.usingSpecularGlossiness = true;
			}
		}
	}
}
