#include "MaterialManager.h"
#include "Managers/TextureManager.h"
#include "Managers/ComponentInterface.h"
#include "OEMaths/OEMaths_transform.h"
#include "Vulkan/VkTextureManager.h"
#include "Managers/EventManager.h"
#include "Engine/Omega_Global.h"

namespace OmegaEngine
{

	MaterialManager::MaterialManager(vk::Device& dev, vk::PhysicalDevice& phys_device, VulkanAPI::Queue& queue) :
		device(dev),
		gpu(phys_device),
		graph_queue(queue)
	{
		// upload empty texture - used when no texture is available for a particular pbr material.
		mapped_blank_tex.create_empty_texture(2048, 2048, true);
		blank_texture.init(device, gpu, graph_queue, VulkanAPI::TextureType::Normal);
		blank_texture.map(mapped_blank_tex);
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
			mat.uvSets.baseColour = gltf_mat.values["baseColorTexture"].TextureTexCoord();
		}
		if (gltf_mat.values.find("metallicRoughnessTexture") != gltf_mat.values.end()) {
			mat.textures[(int)PbrMaterials::MetallicRoughness].image = gltf_mat.values["metallicRoughnessTexture"].TextureIndex();
			mat.textures[(int)PbrMaterials::MetallicRoughness].set = set;
			mat.texture_state[(int)PbrMaterials::MetallicRoughness] = true;
			mat.uvSets.metallicRoughness = gltf_mat.values["metallicRoughnessTexture"].TextureTexCoord();
		}
		if (gltf_mat.values.find("baseColorFactor") != gltf_mat.values.end()) {
			//mat.factors.baseColour = OEMaths::convert_vec4((float*)gltf_mat.values["baseColorFactor"].ColorFactor());
		}
		if (gltf_mat.values.find("metallicFactor") != gltf_mat.values.end()) {
			mat.factors.metallic = static_cast<float>(gltf_mat.values["metallicFactor"].Factor());
		}
		if (gltf_mat.values.find("roughnessFactor") != gltf_mat.values.end()) {
			mat.factors.roughness = static_cast<float>(gltf_mat.values["roughnessFactor"].Factor());
		}

		// any additional textures?
		if (gltf_mat.additionalValues.find("normalTexture") != gltf_mat.additionalValues.end()) {
			mat.textures[(int)PbrMaterials::Normal].image = gltf_mat.additionalValues["normalTexture"].TextureIndex();
			mat.textures[(int)PbrMaterials::Normal].set = set;
			mat.texture_state[(int)PbrMaterials::Normal] = true;
			mat.uvSets.normal = gltf_mat.additionalValues["normalTexture"].TextureTexCoord();
		}
		if (gltf_mat.additionalValues.find("emissiveTexture") != gltf_mat.additionalValues.end()) {
			mat.textures[(int)PbrMaterials::Emissive].image = gltf_mat.additionalValues["emissiveTexture"].TextureIndex();
			mat.textures[(int)PbrMaterials::Emissive].set = set;
			mat.texture_state[(int)PbrMaterials::Emissive] = true;
			mat.uvSets.emissive = gltf_mat.additionalValues["emissiveTexture"].TextureTexCoord();
		}
		if (gltf_mat.additionalValues.find("occlusionTexture") != gltf_mat.additionalValues.end()) {
			mat.textures[(int)PbrMaterials::Occlusion].image = gltf_mat.additionalValues["occlusionTexture"].TextureIndex();
			mat.textures[(int)PbrMaterials::Occlusion].set = set;
			mat.texture_state[(int)PbrMaterials::Occlusion] = true;
			mat.uvSets.occlusion = gltf_mat.additionalValues["occlusionTexture"].TextureTexCoord();
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
			mat.factors.emissive = OEMaths::convert_vec3<float>(gltf_mat.additionalValues["emissiveFactor"].ColorFactor().data());
		}

		// check for extensions
		auto extension = gltf_mat.extensions.find("KHR_materials_pbrSpecularGlossiness");
		if (extension != gltf_mat.extensions.end()) {
			if (extension->second.Has("specularGlossinessTexture")) {
				auto index = extension->second.Get("specularGlossinessTexture").Get("index");
				mat.textures[(int)PbrMaterials::MetallicRoughness].image = index.Get<int>();
				mat.usingSpecularGlossiness = true;

				auto uv_index = extension->second.Get("specularGlossinessTexture").Get("texCoord");
				mat.uvSets.specularGlossiness = uv_index.Get<int>();
			}
			if (extension->second.Has("diffuseTexture")) {
				auto index = extension->second.Get("diffuseTexture").Get("index");
				mat.textures[(int)PbrMaterials::BaseColor].image = index.Get<int>();
				mat.usingSpecularGlossiness = true;

				auto uv_index = extension->second.Get("diffuseTexture").Get("texCoord");
				mat.uvSets.diffuse = uv_index.Get<int>();
			}
			if (extension->second.Has("diffuseFactor")) {
				auto factor = extension->second.Get("diffuseFactor");
			}
			if (extension->second.Has("specularFactor")) {
				auto factor = extension->second.Get("specularFactor");
			}
		}

		materials.push_back(mat);
	}

	MaterialManager::MaterialInfo& MaterialManager::get(uint32_t index)
	{
		assert(index < materials.size());
		return materials[index];
	}

	void MaterialManager::update_frame(double time, double dt, std::unique_ptr<ObjectManager>& obj_manager, ComponentInterface* component_interface)
	{
		if (isDirty) {

			auto& tex_manager = component_interface->getManager<TextureManager>();

			for (auto& mat : materials) {
				
				// all textures are going to be copied over to the the grpahics side
				for (uint8_t i = 0; i < (uint8_t)PbrMaterials::Count; ++i) {
					
					const char* mat_name = create_material_id(mat.name, i);

					// do wew actually have an image for this particular pbr material
					if (mat.texture_state[i]) {

						VulkanAPI::TextureUpdateEvent event{ mat_name, &tex_manager.get_texture(mat.textures[i].set, mat.textures[i].image), tex_manager.get_sampler(mat.textures[i].set, mat.textures[i].sampler) };
						Global::eventManager()->addQueueEvent<VulkanAPI::TextureUpdateEvent>(event);
					}
					else {
						VulkanAPI::TextureUpdateEvent event{ mat_name, &tex_manager.get_empty_texture(), tex_manager.get_empty_sampler() };
						Global::eventManager()->addQueueEvent<VulkanAPI::TextureUpdateEvent>(event);
					}
				}
			}

			for (auto& mat : materials) {

				// init the descriptor set ready for updating with each pbr image element
				mat.descr_set.init(device, descr_layout, descr_pool, 0);

				// map all of the pbr materials for this primitive mesh to the gpu 
				for (uint8_t i = 0; i < (uint8_t)PbrMaterials::Count; ++i) {

					// if we have a texture then map it to the gpu
					
						// not sure this should be done here - should probably be used to the under-used texture manager on the vulkan side
						

						// now update the decscriptor set with the texture info 
						VulkanAPI::SamplerType type = tex_manager.get_sampler(mat.textures[i].set, mat.textures[i].sampler);
						if (type != VulkanAPI::SamplerType::NotDefined) {
							mat.sampler.create(device, type);
						}
						else {
							mat.sampler.create(device, VulkanAPI::SamplerType::Clamp);
						}
					}
					else {
						// otherwise use a blank dummy texture
						mat.vk_textures[i] = blank_texture;
						mat.sampler.create(device, VulkanAPI::SamplerType::Clamp);
					}
						
					// materials always are set = 0 and bindings MUST follow the pbr material sequence
					// sanity check first - make sure their initilialised!
					assert(descr_layout);
					assert(descr_pool);

					
				}
			}
		}

		isDirty = false;
	}
}
