#include "MaterialManager.h"
#include "Managers/TextureManager.h"
#include "OEMaths/OEMaths_transform.h"

namespace OmegaEngine
{

	MaterialManager::MaterialManager()
	{
	}


	MaterialManager::~MaterialManager()
	{
	}

	void MaterialManager::addGltfMaterial(tinygltf::Material& gltf_mat, TextureManager& textureManager)
	{
		MaterialInfo mat;
		// go through each material type and see if they exsist - we are only saving the index
		if (gltf_mat.values.find("baseColorTexture") != gltf_mat.values.end()) {
			mat.textures[(int)PbrMaterials::BaseColor].image = textureManager.get_texture_index("baseColorTexture");
		}
		if (gltf_mat.values.find("metallicRoughnessTexture") != gltf_mat.values.end()) {
			mat.textures[(int)PbrMaterials::MetallicRoughness].image = textureManager.get_texture_index("metallicRoughnessTexture");
		}
		if (gltf_mat.values.find("baseColorFactor") != gltf_mat.values.end()) {
			mat.factors.baseColour = gltf_mat.values["baseColorFactor"].Factor();
		}
		if (gltf_mat.values.find("metallicRoughnessFactor") != gltf_mat.values.end()) {
			mat.factors.metallic = gltf_mat.values["metallicRoughnessFactor"].Factor();
		}

		// any additional textures?
		if (gltf_mat.additionalValues.find("normalTexture") != gltf_mat.additionalValues.end()) {
			mat.textures[(int)PbrMaterials::Normal].image = textureManager.get_texture_index("normalTexture");
		}
		if (gltf_mat.additionalValues.find("emissiveTexture") != gltf_mat.additionalValues.end()) {
			mat.textures[(int)PbrMaterials::Emissive].image = textureManager.get_texture_index("emissiveTexture");
		}
		if (gltf_mat.additionalValues.find("occlusionTexture") != gltf_mat.additionalValues.end()) {
			mat.textures[(int)PbrMaterials::Occlusion].image = textureManager.get_texture_index("occlusionTexture");
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
			mat.factors.alphaMaskCutOff = gltf_mat.additionalValues["alphaCutOff"].Factor();
		}
		if (gltf_mat.additionalValues.find("emissiveFactor") != gltf_mat.additionalValues.end()) {
			mat.factors.emissive = OEMaths::convert_vec3((float*)gltf_mat.additionalValues["emssiveFactor"].ColorFactor().data());
		}

		// check for extensions
		/*if (!mat.extensions.empty()) {

			vkmat.extension = std::make_unique<MaterialExt>();
			if (mat.extensions.find("specularGlossinessTexture") != mat.extensions.end()) {
				vkmat.extension->specularGlossiness = mat.extensions["specularGlossinessTexture"].
			}
		}*/

		materials.push_back(mat);
	}

	MaterialManager::MaterialInfo& MaterialManager::get(uint32_t index)
	{
		assert(index < materials.size());
		return materials[index];
	}

	void MaterialManager::update_frame(double time, double dt, std::unique_ptr<ObjectManager>& obj_manager)
	{
		// if dirty - then upload material to gpu. With other managers, this needs to be adjusted so only materials
		// that are changed/deleted/added are updated
		if (isDirty) {
			
			for (auto& mat : materials) {

				// map all of the pbr materials for this primitive mesh to the gpu 
				for (uint8_t i = 0; i < (uint8_t)PbrMaterials::Count; ++i) {

					vk_textures[i].map(comp_interface->getManager<TextureManager>().get_texture(mat.textures[i].image));

					// now update the decscriptor set with the texture info 
					mat.sampler->create(device, comp_interface->getManager<TextureManager>().get_sampler(mat.textures[i].sampler));

					// materials always are set = 0 and bindings follow the pbr material sequence - no reflection used
					mat.descr_set.update_set(0, i, vk::DescriptorType::eSampler, mat.sampler->get_sampler(), vk_textures[i].get_image_view(), vk::ImageLayout::eColorAttachmentOptimal);

				}
			}
		}	
		
		isDirty = false;
	}

}
