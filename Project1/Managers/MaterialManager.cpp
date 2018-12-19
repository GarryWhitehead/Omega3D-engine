#include "MaterialManager.h"

namespace OmegaEngine
{

	MaterialManager::MaterialManager()
	{
	}


	MaterialManager::~MaterialManager()
	{
	}

	void MaterialManager::parseGltfFile(uint32_t spaceId, tinygltf::Model& model)
	{
		std::vector<Material> matVec;

		for (tinygltf::Material mat : model.materials) {

			Material vkmat;
			// go through each material type and see if they exsist - we are only saving the index - we can then use this and the space id to get the required textures when needed
			if (mat.values.find("baseColorTexture") != mat.values.end()) {
				vkmat.textureIndicies.baseColor = mat.values["baseColorTexture"].TextureIndex();
			}
			if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {
				vkmat.textureIndicies.metallicRoughness = mat.values["metallicRoughnessTexture"].TextureIndex();
			}
			if (mat.values.find("baseColorFactor") != mat.values.end()) {
				vkmat.baseColorFactor = mat.values["baseColorFactor"].Factor();
			}
			if (mat.values.find("metallicRoughnessFactor") != mat.values.end()) {
				vkmat.metallicFactor = mat.values["metallicRoughnessFactor"].Factor();
			}

			// any additional textures?
			if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
				vkmat.textureIndicies.normal = mat.additionalValues["normalTexture"].TextureIndex();
			}
			if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end()) {
				vkmat.textureIndicies.emissive = mat.additionalValues["emissiveTexture"].TextureIndex();
			}
			if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end()) {
				vkmat.textureIndicies.occlusion = mat.additionalValues["occlusionTexture"].TextureIndex();
			}

			// check for aplha modes
			if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {
				tinygltf::Parameter param = mat.additionalValues["alphaMode"];
				if (param.string_value == "BLEND") {
					vkmat.alphaMode = AlphaMode::Blend;
				}
				if (param.string_value == "MASK") {
					vkmat.alphaMode = AlphaMode::Mask;
				}
			}
			if (mat.additionalValues.find("alphaCutOff") != mat.additionalValues.end()) {
				vkmat.alphaCutOff = mat.additionalValues["alphaCutOff"].Factor();
			}
			if (mat.additionalValues.find("emissiveFactor") != mat.additionalValues.end()) {
				vkmat.emmisiveFactor = glm::make_vec3(mat.additionalValues["emssiveFactor"].ColorFactor().data());
			}

			// check for extensions
			/*if (!mat.extensions.empty()) {
				
				vkmat.extension = std::make_unique<MaterialExt>();
				if (mat.extensions.find("specularGlossinessTexture") != mat.extensions.end()) {
					vkmat.extension->specularGlossiness = mat.extensions["specularGlossinessTexture"].
				}
			}*/

			matVec.push_back(vkmat);
		}
		
		materials.insert(std::make_pair(spaceId, matVec));
	}
}
