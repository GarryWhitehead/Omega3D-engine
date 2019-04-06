#include "VkTextureManager.h"
#include "Vulkan/DataTypes/Texture.h"

#include "Engine/Omega_Global.h"

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

		textures[event.id] = tex_info;
	}

	void VkTextureManager::update_descriptors()
	{
		mat.descr_set.write_set(0, i, vk::DescriptorType::eCombinedImageSampler, mat.sampler.get_sampler(), mat.vk_textures[i].get_image_view(), vk::ImageLayout::eShaderReadOnlyOptimal);
	}
	
}