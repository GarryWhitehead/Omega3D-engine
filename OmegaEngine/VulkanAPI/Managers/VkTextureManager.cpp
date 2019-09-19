#include "VkTextureManager.h"

#include "Core/Omega_Global.h"

#include "Utility/logger.h"

#include "Types/ImageReflect.h"

#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Types/Texture.h"

namespace VulkanAPI
{

VkTextureManager::VkTextureManager(vk::Device& dev, vk::PhysicalDevice& physicalDevice, VulkanAPI::Queue& queue)
    : device(dev)
    , gpu(physicalDevice)
    , graphicsQueue(queue)
{
	OmegaEngine::Global::eventManager()
	    ->registerListener<VkTextureManager, TextureUpdateEvent, &VkTextureManager::updateTexture>(this);
	OmegaEngine::Global::eventManager()
	    ->registerListener<VkTextureManager, MaterialTextureUpdateEvent, &VkTextureManager::updateGroupedTexture>(this);
}

VkTextureManager::~VkTextureManager()
{
}

void VkTextureManager::updateGroupedTexture(MaterialTextureUpdateEvent& event)
{
	assert(event.mappedTexture != nullptr);

	MaterialTextureInfo tex_info;
	tex_info.texture.init(device, gpu, graphicsQueue);
	tex_info.texture.map(*event.mappedTexture);
	tex_info.sampler.create(device, event.sampler);
	tex_info.binding = event.binding;

	groupedTextures[event.id].push_back(tex_info);
}

void VkTextureManager::updateGroupedDescriptors()
{

	for (auto& descr : descriptorSetUpdateQueue)
	{
		assert(descr.set != nullptr);
		MaterialTextureInfo texture = groupedTextures[descr.id][descr.binding];
		descr.set->writeSet(descr.setValue, descr.binding, vk::DescriptorType::eCombinedImageSampler,
		                    texture.sampler.getSampler(), texture.texture.getImageView(),
		                    vk::ImageLayout::eShaderReadOnlyOptimal);
	}
}

void VkTextureManager::bindTexturesToDescriptorLayout(const char* id, DescriptorLayout* layout, uint32_t setValue)
{
	assert(layout != nullptr);

	auto iter = textureLayouts.begin();
	while (iter != textureLayouts.end())
	{
		if (std::strcmp(iter->first, id) == 0)
		{
			break;
		}
		iter++;
	}

	if (iter != textureLayouts.end())
	{
		LOGGER_INFO("Texture layout binding of id %s was already registered.\n", id);
	}
	textureLayouts[id] = { layout, setValue };
}

VkTextureManager::TextureLayoutInfo& VkTextureManager::getTextureDescriptorLayout(const char* id)
{
	auto iter = textureLayouts.begin();
	while (iter != textureLayouts.end())
	{

		if (std::strcmp(iter->first, id) == 0)
		{
			break;
		}
		iter++;
	}

	if (iter == textureLayouts.end())
	{
		LOGGER_ERROR("Layout with id %s was not registered with vulkan texture manager.\n", id);
	}
	return iter->second;
}

bool VkTextureManager::prepareGroupedSet(DescriptorSet& set, Util::String id, uint32_t setValue)
{
	auto iter = groupedTextures.find(id);

	if (iter == groupedTextures.end())
	{
		LOGGER_ERROR("An id of %s has not been registered with the vulkan texture manager.\n", id);
		return false;
	}

	for (auto& texture : iter->second)
	{
		set.writeSet(setValue, texture.binding, vk::DescriptorType::eCombinedImageSampler, texture.sampler.getSampler(),
		             texture.texture.getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
	}
}

void VkTextureManager::updateTexture(TextureUpdateEvent& event)
{
	assert(event.textureInfo != nullptr);

	TextureInfo tex_info;
	tex_info.texture.init(device, gpu, graphicsQueue);
	tex_info.texture.map(event.textureInfo->texture);
	tex_info.sampler.create(device, event.textureInfo->samplerType);

	textures[event.id.c_str()] = tex_info;
}

bool VkTextureManager::getTextureImageView(Util::String name, vk::ImageView& outputView)
{
	auto iter = textures.find(name);

	if (iter == textures.end())
	{
		LOGGER_ERROR("Unable to find texture with id: %s.\n", name);
	}

	return iter->second.texture.getImageView();
}

bool VkTextureManager::prepareDescriptors(ImageReflect& reflected, DescriptorSet& descrSet)
{
	if (reflected.layouts.empty())
	{
		return true;
	}

	for (auto& reflect : reflected.layouts)
	{
		auto iter = textures.find(reflect.name);

		if (iter != textures.end())
		{
			descrSet.writeSet(reflect.set, reflect.binding, vk::DescriptorType::eCombinedImageSampler, reflect.sampler,
			                  iter->second.texture.getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
		}
	}
}

}    // namespace VulkanAPI