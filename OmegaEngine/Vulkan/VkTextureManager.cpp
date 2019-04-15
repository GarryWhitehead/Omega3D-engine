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
		TextureInfo tex_info;
		tex_info.texture.init(device, gpu, graph_queue, VulkanAPI::TextureType::Normal);
		tex_info.texture.map(*event.mapped_tex);
		tex_info.sampler.create(device, event.sampler);

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
		if (textures.find(id) == textures.end()) {
			LOGGER_ERROR("An id of %s has not been registered with the vulkan texture manager.\n", id);
		}

		for (auto& texture : textures[id]) {
			set.write_set(set_num, texture.binding, vk::DescriptorType::eCombinedImageSampler, texture.sampler.get_sampler(), texture.texture.get_image_view(), vk::ImageLayout::eShaderReadOnlyOptimal);
		}
	}

	void VkTextureManager::bind_textures_to_layout(const char* id, DescriptorLayout* layout)
	{
		assert(layout != nullptr);

		if (texture_layouts.find(id) != texture_layouts.end()) {
			LOGGER_INFO("Texture layout binding of id %s was already addeded.\n", id);
		}
		texture_layouts[id] = layout;
	}
}