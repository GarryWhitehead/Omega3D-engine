#include "MaterialManager.h"
#include "Managers/TextureManager.h"
#include "Managers/ComponentInterface.h"
#include "OEMaths/OEMaths_transform.h"

namespace OmegaEngine
{

	MaterialManager::MaterialManager(vk::Device& dev, vk::PhysicalDevice& phys_device, VulkanAPI::Queue& queue) :
		device(dev),
		gpu(phys_device),
		graph_queue(queue)
	{
	}


	MaterialManager::~MaterialManager()
	{
	}

	void MaterialManager::addGltfMaterial(uint32_t set, tinygltf::Material& gltf_mat, TextureManager& textureManager)
	{
		MaterialInfo mat;
		// go through each material type and see if they exsist - we are only saving the index
		if (gltf_mat.values.find("baseColorTexture") != gltf_mat.values.end()) {
			mat.textures[(int)PbrMaterials::BaseColor].image = gltf_mat.values["baseColorTexture"].TextureIndex();
			mat.textures[(int)PbrMaterials::BaseColor].set = set;
			mat.texture_state[(int)PbrMaterials::BaseColor] = true;
		}
		if (gltf_mat.values.find("metallicRoughnessTexture") != gltf_mat.values.end()) {
			mat.textures[(int)PbrMaterials::MetallicRoughness].image = gltf_mat.values["metallicRoughnessTexture"].TextureIndex();
			mat.textures[(int)PbrMaterials::MetallicRoughness].set = set;
			mat.texture_state[(int)PbrMaterials::MetallicRoughness] = true;
		}
		if (gltf_mat.values.find("baseColorFactor") != gltf_mat.values.end()) {
			mat.factors.baseColour = static_cast<float>(gltf_mat.values["baseColorFactor"].Factor());
		}
		if (gltf_mat.values.find("metallicRoughnessFactor") != gltf_mat.values.end()) {
			mat.factors.metallic = static_cast<float>(gltf_mat.values["metallicRoughnessFactor"].Factor());
		}

		// any additional textures?
		if (gltf_mat.additionalValues.find("normalTexture") != gltf_mat.additionalValues.end()) {
			mat.textures[(int)PbrMaterials::Normal].image = gltf_mat.values["normalTexture"].TextureIndex();
			mat.textures[(int)PbrMaterials::Normal].set = set;
			mat.texture_state[(int)PbrMaterials::Normal] = true;
		}
		if (gltf_mat.additionalValues.find("emissiveTexture") != gltf_mat.additionalValues.end()) {
			mat.textures[(int)PbrMaterials::Emissive].image = gltf_mat.values["emissiveTexture"].TextureIndex();
			mat.textures[(int)PbrMaterials::Emissive].set = set;
			mat.texture_state[(int)PbrMaterials::Emissive] = true;
		}
		if (gltf_mat.additionalValues.find("occlusionTexture") != gltf_mat.additionalValues.end()) {
			mat.textures[(int)PbrMaterials::Occlusion].image = gltf_mat.values["occlusionTexture"].TextureIndex();
			mat.textures[(int)PbrMaterials::Occlusion].set;
			mat.texture_state[(int)PbrMaterials::Occlusion] = true;
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
			mat.factors.alphaMaskCutOff = static_cast<float>(gltf_mat.additionalValues["alphaCutOff"].Factor());
		}
		if (gltf_mat.additionalValues.find("emissiveFactor") != gltf_mat.additionalValues.end()) {
			mat.factors.emissive = OEMaths::convert_vec3<double>(gltf_mat.additionalValues["emissiveFactor"].ColorFactor().data());
		}

		// check for extensions
		

		materials.push_back(mat);
	}

	MaterialManager::MaterialInfo& MaterialManager::get(uint32_t index)
	{
		assert(index < materials.size());
		return materials[index];
	}

	void MaterialManager::update_frame(double time, double dt, std::unique_ptr<ObjectManager>& obj_manager, ComponentInterface* component_interface)
	{
		// if dirty - then upload material to gpu. With other managers, this needs to be adjusted so only materials
		// that are changed/deleted/added are updated
		if (isDirty) {
			
			auto& tex_manager = component_interface->getManager<TextureManager>();

			for (auto& mat : materials) {

				// map all of the pbr materials for this primitive mesh to the gpu 
				for (uint8_t i = 0; i < (uint8_t)PbrMaterials::Count; ++i) {

					if (mat.texture_state[i]) {
						// not sure this should be done here - should probably be used to the under-used texture manager on the vulkan side
						mat.vk_textures[i].init(device, gpu, graph_queue, VulkanAPI::TextureType::Normal);
						mat.vk_textures[i].map(tex_manager.get_texture(mat.textures[i].set, mat.textures[i].image));

						// now update the decscriptor set with the texture info 
						VulkanAPI::SamplerType type = tex_manager.get_sampler(mat.textures[i].set, mat.textures[i].sampler);
						if (type != VulkanAPI::SamplerType::NotDefined) {
							mat.sampler.create(device, type);
						}
						else {
							mat.sampler.create(device, VulkanAPI::SamplerType::Clamp);
						}

						// materials always are set = 0 and bindings MUST follow the pbr material sequence
						// sanity check first - make sure their initilialised!
						assert(descr_layout);
						assert(descr_pool);
						mat.descr_set.init(device, descr_layout, descr_pool, 0);
						mat.descr_set.write_set(0, i, vk::DescriptorType::eCombinedImageSampler, mat.sampler.get_sampler(), mat.vk_textures[i].get_image_view(), vk::ImageLayout::eColorAttachmentOptimal);
					}
				}
			}
		}	
		
		isDirty = false;
	}

}
