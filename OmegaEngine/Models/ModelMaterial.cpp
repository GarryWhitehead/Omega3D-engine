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

	void ModelMaterial::extractMaterialData(tinygltf::Material& gltfMaterial)
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
