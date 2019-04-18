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
	}


	VkTextureManager::~VkTextureManager()
	{
	}

	void VkTextureManager::update_texture(TextureUpdateEvent& event)
	{
		assert(event.mapped_tex != nullptr);

		TextureInfo tex_info;
		tex_info.texture.init(device, gpu, graph_queue, VulkanAPI::TextureType::Normal);
		tex_info.texture.map(*event.mapped_tex);
		tex_info.sampler.create(device, event.sampler);
		tex_info.binding = event.binding;

		textures[event.id].push_back(tex_info);
	}

	void VkTextureManager::update_descriptors()
	{
		
		for (auto& descr : descr_set_update_queue) {
			
			assert(descr.set != nullptr);
			TextureInfo texture = textures[descr.id][descr.binding];
			descr.set->write_set(descr.set_num, descr.binding, vk::DescriptorType::eCombinedImageSampler, texture.sampler.get_sampler(), texture.texture.get_image_view(), vk::ImageLayout::eShaderReadOnlyOptimal);
		}
	}
	
	void VkTextureManager::update_descr_set(DescriptorSet& set, const char* id, uint32_t set_num)
	{
		auto iter = textures.begin();
		while (iter != textures.end()) {

			if (iter->first == id) {
				break;
			}
			iter++;
		}

		if (iter == textures.end()) {
			LOGGER_ERROR("An id of %s has not been registered with the vulkan texture manager.\n", id);
		}
		
		for (auto& texture : iter->second) {
			set.write_set(set_num, texture.binding, vk::DescriptorType::eCombinedImageSampler, texture.sampler.get_sampler(), texture.texture.get_image_view(), vk::ImageLayout::eShaderReadOnlyOptimal);
		}
	}

	void VkTextureManager::bind_textures_to_layout(const char* id, DescriptorLayout* layout, uint32_t set_num)
	{
		assert(layout != nullptr);

		auto iter = texture_layouts.begin();
		while (iter != texture_layouts.end()) {

			if (std::strcmp(iter->first,id) == 0) {
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
}