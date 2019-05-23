#include "MaterialManager.h"
#include "Managers/TextureManager.h"
#include "Managers/ComponentInterface.h"
#include "OEMaths/OEMaths_transform.h"
#include "Vulkan/VkTextureManager.h"
#include "Managers/EventManager.h"
#include "Engine/Omega_Global.h"

namespace OmegaEngine
{

	MaterialManager::MaterialManager() 
	{
	}


	MaterialManager::~MaterialManager()
	{
	}

	void MaterialManager::addGltfMaterial(uint32_t set, tinygltf::Material& gltfMaterial, TextureManager& textureManager)
	{
		MaterialInfo mat;
		mat.name = _strdup(gltfMaterial.name.c_str());

		// go through each material type and see if they exsist - we are only saving the index
		if (gltfMaterial.values.find("baseColorTexture") != gltfMaterial.values.end()) 
		{
			mat.textures[(int)PbrMaterials::BaseColor].image = gltfMaterial.values["baseColorTexture"].TextureIndex();
			mat.textures[(int)PbrMaterials::BaseColor].set = set;
			mat.textureState[(int)PbrMaterials::BaseColor] = true;
			mat.uvSets.baseColour = gltfMaterial.values["baseColorTexture"].TextureTexCoord();
		}
		if (gltfMaterial.values.find("metallicRoughnessTexture") != gltfMaterial.values.end()) 
		{
			mat.textures[(int)PbrMaterials::MetallicRoughness].image = gltfMaterial.values["metallicRoughnessTexture"].TextureIndex();
			mat.textures[(int)PbrMaterials::MetallicRoughness].set = set;
			mat.textureState[(int)PbrMaterials::MetallicRoughness] = true;
			mat.uvSets.metallicRoughness = gltfMaterial.values["metallicRoughnessTexture"].TextureTexCoord();
		}
		if (gltfMaterial.values.find("baseColorFactor") != gltfMaterial.values.end()) 
		{
			mat.factors.baseColour = OEMaths::vec4f(gltfMaterial.values["baseColorFactor"].ColorFactor().data());
		}
		if (gltfMaterial.values.find("metallicFactor") != gltfMaterial.values.end()) 
		{
			mat.factors.metallic = static_cast<float>(gltfMaterial.values["metallicFactor"].Factor());
		}
		if (gltfMaterial.values.find("roughnessFactor") != gltfMaterial.values.end()) 
		{
			mat.factors.roughness = static_cast<float>(gltfMaterial.values["roughnessFactor"].Factor());
		}

		// any additional textures?
		if (gltfMaterial.additionalValues.find("normalTexture") != gltfMaterial.additionalValues.end()) 
		{
			mat.textures[(int)PbrMaterials::Normal].image = gltfMaterial.additionalValues["normalTexture"].TextureIndex();
			mat.textures[(int)PbrMaterials::Normal].set = set;
			mat.textureState[(int)PbrMaterials::Normal] = true;
			mat.uvSets.normal = gltfMaterial.additionalValues["normalTexture"].TextureTexCoord();
		}
		if (gltfMaterial.additionalValues.find("emissiveTexture") != gltfMaterial.additionalValues.end()) 
		{
			mat.textures[(int)PbrMaterials::Emissive].image = gltfMaterial.additionalValues["emissiveTexture"].TextureIndex();
			mat.textures[(int)PbrMaterials::Emissive].set = set;
			mat.textureState[(int)PbrMaterials::Emissive] = true;
			mat.uvSets.emissive = gltfMaterial.additionalValues["emissiveTexture"].TextureTexCoord();
		}
		if (gltfMaterial.additionalValues.find("occlusionTexture") != gltfMaterial.additionalValues.end()) 
		{
			mat.textures[(int)PbrMaterials::Occlusion].image = gltfMaterial.additionalValues["occlusionTexture"].TextureIndex();
			mat.textures[(int)PbrMaterials::Occlusion].set = set;
			mat.textureState[(int)PbrMaterials::Occlusion] = true;
			mat.uvSets.occlusion = gltfMaterial.additionalValues["occlusionTexture"].TextureTexCoord();
		}

		// check for aplha modes
		if (gltfMaterial.additionalValues.find("alphaMode") != gltfMaterial.additionalValues.end()) 
		{
			tinygltf::Parameter param = gltfMaterial.additionalValues["alphaMode"];
			if (param.string_value == "BLEND") 
			{
				mat.factors.alphaMask = MaterialInfo::AlphaMode::Blend;
			}
			if (param.string_value == "MASK") 
			{
				mat.factors.alphaMask = MaterialInfo::AlphaMode::Mask;
			}
		}
		if (gltfMaterial.additionalValues.find("alphaCutOff") != gltfMaterial.additionalValues.end()) 
		{
			mat.factors.alphaMaskCutOff = static_cast<float>(gltfMaterial.additionalValues["alphaCutOff"].Factor());
		}
		if (gltfMaterial.additionalValues.find("emissiveFactor") != gltfMaterial.additionalValues.end())
		{
			mat.factors.emissive = OEMaths::vec3f(gltfMaterial.additionalValues["emissiveFactor"].ColorFactor().data());
		}

		// check for extensions
		auto extension = gltfMaterial.extensions.find("KHR_materials_pbrSpecularGlossiness");
		if (extension != gltfMaterial.extensions.end()) 
		{
			if (extension->second.Has("specularGlossinessTexture")) 
			{
				auto index = extension->second.Get("specularGlossinessTexture").Get("index");
				mat.textures[(int)PbrMaterials::MetallicRoughness].image = index.Get<int>();
				mat.usingSpecularGlossiness = true;

				auto uv_index = extension->second.Get("specularGlossinessTexture").Get("texCoord");
				mat.uvSets.specularGlossiness = uv_index.Get<int>();
			}
			if (extension->second.Has("diffuseTexture")) 
			{
				auto index = extension->second.Get("diffuseTexture").Get("index");
				mat.textures[(int)PbrMaterials::BaseColor].image = index.Get<int>();
				mat.usingSpecularGlossiness = true;

				auto uvIndex = extension->second.Get("diffuseTexture").Get("texCoord");
				mat.uvSets.diffuse = uvIndex.Get<int>();
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

				mat.factors.diffuse = OEMaths::vec4f(x, y, z, w);
				mat.usingSpecularGlossiness = true;
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

				mat.factors.specular = OEMaths::vec3f(x, y, z);
				mat.usingSpecularGlossiness = true;
			}
		}

		materials.push_back(mat);
	}

	MaterialInfo& MaterialManager::get(uint32_t index)
	{
		assert(index < materials.size());
		return materials[index];
	}

	void MaterialManager::updateFrame(double time, double dt, std::unique_ptr<ObjectManager>& objectManager, ComponentInterface* componentInterface)
	{
		if (isDirty) 
		{
			auto& textureManager = componentInterface->getManager<TextureManager>();

			for (auto& mat : materials) 
			{
				// all textures are going to be copied over to the the grpahics side
				for (uint8_t i = 0; i < (uint8_t)PbrMaterials::Count; ++i) {

					// do we actually have an image for this particular pbr material
					if (mat.textureState[i]) 
					{
						VulkanAPI::MaterialTextureUpdateEvent event{ mat.name, i, &textureManager.getTexture(mat.textures[i].set, mat.textures[i].image),
							textureManager.getSampler(mat.textures[i].set, mat.textures[i].sampler) };
						Global::eventManager()->addQueueEvent<VulkanAPI::MaterialTextureUpdateEvent>(event);
					}
					else 
					{
						VulkanAPI::MaterialTextureUpdateEvent event{ mat.name, i, &textureManager.getDummyTexture(), textureManager.getDummySampler() };
						Global::eventManager()->addQueueEvent<VulkanAPI::MaterialTextureUpdateEvent>(event);
					}
				}
			}
		}

		isDirty = false;
	}
}
