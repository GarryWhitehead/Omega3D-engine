#include "MaterialManager.h"

namespace OmegaEngine
{

	MaterialManager::MaterialManager()
	{
	}


	MaterialManager::~MaterialManager()
	{
	}

	void MaterialManager::addGltfMaterial(tinygltf::Material& gltf_mat)
	{
		MaterialInfo mat;
		// go through each material type and see if they exsist - we are only saving the index - we can then use this and the space id to get the required textures when needed
		if (gltf_mat.values.find("baseColorTexture") != gltf_mat.values.end()) {
			mat.baseColorIndex = gltf_mat.values["baseColorTexture"].TextureIndex();
		}
		if (gltf_mat.values.find("metallicRoughnessTexture") != gltf_mat.values.end()) {
			mat.metallicRoughnessIndex = gltf_mat.values["metallicRoughnessTexture"].TextureIndex();
		}
		if (gltf_mat.values.find("baseColorFactor") != gltf_mat.values.end()) {
			mat.pushBlock.baseColorFactor = gltf_mat.values["baseColorFactor"].Factor();
		}
		if (gltf_mat.values.find("metallicRoughnessFactor") != gltf_mat.values.end()) {
			mat.pushBlock.metallicFactor = gltf_mat.values["metallicRoughnessFactor"].Factor();
		}

		// any additional textures?
		if (gltf_mat.additionalValues.find("normalTexture") != gltf_mat.additionalValues.end()) {
			mat.normalIndex = gltf_mat.additionalValues["normalTexture"].TextureIndex();
		}
		if (gltf_mat.additionalValues.find("emissiveTexture") != gltf_mat.additionalValues.end()) {
			mat.emissiveIndex = gltf_mat.additionalValues["emissiveTexture"].TextureIndex();
		}
		if (gltf_mat.additionalValues.find("occlusionTexture") != gltf_mat.additionalValues.end()) {
			mat.occlusionIndex = gltf_mat.additionalValues["occlusionTexture"].TextureIndex();
		}

		// check for aplha modes
		if (gltf_mat.additionalValues.find("alphaMode") != gltf_mat.additionalValues.end()) {
			tinygltf::Parameter param = gltf_mat.additionalValues["alphaMode"];
			if (param.string_value == "BLEND") {
				mat.alphaMode = MaterialInfo::AlphaMode::Blend;
			}
			if (param.string_value == "MASK") {
				mat.alphaMode = MaterialInfo::AlphaMode::Mask;
			}
		}
		if (gltf_mat.additionalValues.find("alphaCutOff") != gltf_mat.additionalValues.end()) {
			mat.pushBlock.alphaCutOff = gltf_mat.additionalValues["alphaCutOff"].Factor();
		}
		if (gltf_mat.additionalValues.find("emissiveFactor") != gltf_mat.additionalValues.end()) {
			mat.pushBlock.emissiveFactor = OEMaths::convert_vec3((float*)gltf_mat.additionalValues["emssiveFactor"].ColorFactor().data());
		}

		// check for extensions
		/*if (!mat.extensions.empty()) {

			vkmat.extension = std::make_unique<MaterialExt>();
			if (mat.extensions.find("specularGlossinessTexture") != mat.extensions.end()) {
				vkmat.extension->specularGlossiness = mat.extensions["specularGlossinessTexture"].
			}
		}*/

		materials.insert(std::make_pair(gltf_mat.name.c_str(), mat));
	}
}
