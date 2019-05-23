#include "VkTextureManager.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Vulkan/Descriptors.h"
#include "Engine/Omega_Global.h"
#include "Utility/logger.h"

namespace VulkanAPI
{

	VkTextureManager::VkTextureManager(vk::Device& dev, vk::PhysicalDevice& phys_dev, VulkanAPI::Queue& queue) :
		device(dev),
		gpu(phys_dev),
		graph_queue(queue)
	{
		OmegaEngine::Global::eventManager()->registerListener<VkTextureManager, TextureUpdateEvent, &VkTextureManager::update_texture>(this);
		OmegaEngine::Global::eventManager()->registerListener<VkTextureManager, MaterialTextureUpdateEvent, &VkTextureManager::update_material_texture>(this);
	}


	VkTextureManager::~VkTextureManager()
	{
	}

	void VkTextureManager::update_material_texture(MaterialTextureUpdateEvent& event)
	{
		assert(event.mapped_tex != nullptr);

		MaterialTextureInfo tex_info;
		tex_info.texture.init(device, gpu, graph_queue);
		tex_info.texture.map(*event.mapped_tex);
		tex_info.sampler.create(device, event.sampler);
		tex_info.binding = event.binding;

		mat_textures[event.id].push_back(tex_info);
	}

	void VkTextureManager::update_material_descriptors()
	{

		for (auto& descr : descriptorSet_update_queue) {

			assert(descr.set != nullptr);
			MaterialTextureInfo texture = mat_textures[descr.id][descr.binding];
			descr.set->writeSet(descr.set_num, descr.binding, vk::DescriptorType::eCombinedImageSampler, texture.sampler.getSampler(), texture.texture.getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
		}
	}

	void VkTextureManager::bindTexturesToDescriptorLayout(const char* id, DescriptorLayout* layout, uint32_t set_num)
	{
		assert(layout != nullptr);

		auto iter = texture_layouts.begin();
		while (iter != texture_layouts.end()) {

			if (std::strcmp(iter->first, id) == 0) {
				break;
			}
			iter++;
		}

		if (iter != texture_layouts.end()) {
			LOGGER_INFO("Texture layout binding of id %s was already registered.\n", id);
		}
		texture_layouts[id] = { layout, set_num };
	}

	VkTextureManager::TextureLayoutInfo& VkTextureManager::getTexture_descriptorLayout(const char* id)
	{
		auto iter = texture_layouts.begin();
		while (iter != texture_layouts.end()) {

			if (std::strcmp(iter->first, id) == 0) {
				break;
			}
			iter++;
		}

		if (iter == texture_layouts.end()) {
			LOGGER_ERROR("Layout with id %s was not registered with vulkan texture manager.\n", id);
		}
		return iter->second;
	}

	void VkTextureManager::update_material_descriptorSet(DescriptorSet& set, const char* id, uint32_t set_num)
	{
		auto iter = mat_textures.begin();
		while (iter != mat_textures.end()) {

			if (iter->first == id) {
				break;
			}
			iter++;
		}

		if (iter == mat_textures.end()) {
			LOGGER_ERROR("An id of %s has not been registered with the vulkan texture manager.\n", id);
		}

		for (auto& texture : iter->second) {
			set.writeSet(set_num, texture.binding, vk::DescriptorType::eCombinedImageSampler, texture.sampler.getSampler(), texture.texture.getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
		}
	}

	void VkTextureManager::update_texture(TextureUpdateEvent& event)
	{
		assert(event.mapped_tex != nullptr);

		TextureInfo tex_info;
		tex_info.texture.init(device, gpu, graph_queue);
		tex_info.texture.map(*event.mapped_tex);

		textures[event.id.c_str()] = tex_info;
	}

	void VkTextureManager::enqueueDescrUpdate(const char* id, VulkanAPI::DescriptorSet* descriptorSet, VulkanAPI::Sampler* sampler, uint32_t set, uint32_t binding)
	{
		descriptorSet_update_queue.push_back({ id, descriptorSet, sampler, set, binding });
	}

	void VkTextureManager::update_descriptors()
	{

		if (!descriptorSet_update_queue.empty()) {

			for (auto& descr : descriptorSet_update_queue) {

				auto iter = textures.begin();
				while (iter != textures.end()) {

					if (std::strcmp(iter->first, descr.id) == 0) {
						break;
					}
					iter++;
				}

				if (iter != textures.end()) {

					descr.set->writeSet(descr.set_num, descr.binding, vk::DescriptorType::eCombinedImageSampler, descr.sampler->getSampler(), iter->second.texture.getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
				}
			}
		}

		descriptorSet_update_queue.clear();
	}

	void VkTextureManager::update()
	{
		update_descriptors();
	}
}