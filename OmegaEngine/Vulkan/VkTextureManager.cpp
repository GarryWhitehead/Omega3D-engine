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
		tex_info.texture.init(device, gpu, graph_queue, VulkanAPI::TextureType::Normal);
		tex_info.texture.map(*event.mapped_tex);
		tex_info.sampler.create(device, event.sampler);
		tex_info.binding = event.binding;

		mat_textures[event.id].push_back(tex_info);
	}

	void VkTextureManager::update_material_descriptors()
	{

		for (auto& descr : descr_set_update_queue) {

			assert(descr.set != nullptr);
			MaterialTextureInfo texture = mat_textures[descr.id][descr.binding];
			descr.set->write_set(descr.set_num, descr.binding, vk::DescriptorType::eCombinedImageSampler, texture.sampler.get_sampler(), texture.texture.get_image_view(), vk::ImageLayout::eShaderReadOnlyOptimal);
		}
	}

	void VkTextureManager::bind_textures_to_layout(const char* id, DescriptorLayout* layout, uint32_t set_num)
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

	VkTextureManager::TextureLayoutInfo& VkTextureManager::get_texture_descr_layout(const char* id)
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

	void VkTextureManager::update_material_descr_set(DescriptorSet& set, const char* id, uint32_t set_num)
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
			set.write_set(set_num, texture.binding, vk::DescriptorType::eCombinedImageSampler, texture.sampler.get_sampler(), texture.texture.get_image_view(), vk::ImageLayout::eShaderReadOnlyOptimal);
		}
	}

	void VkTextureManager::update_texture(TextureUpdateEvent& event)
	{
		assert(event.mapped_tex != nullptr);

		TextureInfo tex_info;
		tex_info.texture.init(device, gpu, graph_queue, VulkanAPI::TextureType::Normal);
		tex_info.texture.map(*event.mapped_tex);

		textures[event.id.c_str()] = tex_info;
	}

	void VkTextureManager::enqueueDescrUpdate(const char* id, VulkanAPI::DescriptorSet* descr_set, VulkanAPI::Sampler* sampler, uint32_t set, uint32_t binding)
	{
		descr_set_update_queue.push_back({ id, descr_set, sampler, set, binding });
	}

	void VkTextureManager::update_descriptors()
	{

		if (!descr_set_update_queue.empty()) {

			for (auto& descr : descr_set_update_queue) {

				auto iter = textures.begin();
				while (iter != textures.end()) {

					if (std::strcmp(iter->first, descr.id) == 0) {
						break;
					}
					iter++;
				}

				if (iter != textures.end()) {

					descr.set->write_set(descr.set_num, descr.binding, vk::DescriptorType::eCombinedImageSampler, descr.sampler->get_sampler(), iter->second.texture.get_image_view(), vk::ImageLayout::eShaderReadOnlyOptimal);
				}
			}
		}

		descr_set_update_queue.clear();
	}
}