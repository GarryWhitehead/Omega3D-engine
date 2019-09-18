#pragma once

#include "Managers/AssetManager.h"
#include "Types/MappedTexture.h"

#include "Managers/EventManager.h"
#include "VulkanAPI/Common.h"

#include "VulkanAPI/Types/Texture.h"

#include "VulkanAPI/Sampler.h"
#include "VulkanAPI/VkContext.h"

#include "Utility/String.h"

#include <tuple>
#include <unordered_map>

namespace VulkanAPI
{
// forward declerations
class DescriptorSet;
class DescriptorLayout;
enum class TextureType;
class Texture;
class Queue;
class ImageReflect;

class VkTextureManager
{

public:
	struct MaterialTextureInfo
	{
		Texture texture;
		Sampler sampler;
		uint32_t binding = 0;
	};

	struct TextureInfo
	{
		Texture texture;
		Sampler sampler;
	};

	struct TextureLayoutInfo
	{
		DescriptorLayout* layout = nullptr;
		uint32_t setValue;
	};

	struct DescrSetUpdateInfo
	{
		const char* id;
		DescriptorSet* set = nullptr;
		Sampler* sampler = nullptr;
		uint32_t setValue = 0;
		uint32_t binding = 0;
	};

	VkTextureManager(vk::Device& dev, vk::PhysicalDevice& physicalDevice, VulkanAPI::Queue& queue);
	~VkTextureManager();

	void updateTexture(TextureUpdateEvent& event);
	void enqueueDescrUpdate(const char*, VulkanAPI::DescriptorSet*, VulkanAPI::Sampler* sampler, uint32_t set,
	                        uint32_t binding);

	bool prepareDescriptors(ImageReflect& reflect, DescriptorSet& descrSet);

	// updates a single descriptor set with a texture set identified by its unique id
	bool prepareGroupedSet(DescriptorSet& set, Util::String id, uint32_t setValue);

	void updateGroupedTexture(MaterialTextureUpdateEvent& event);
	void updateGroupedDescriptors();

	// associates an id with a descriptor layout. Used for materials, etc. were there are multiple descriptor sets but one layout
	void bindTexturesToDescriptorLayout(const char* id, DescriptorLayout* layout, uint32_t setValue);

	TextureLayoutInfo& getTextureDescriptorLayout(const char* id);

	vk::ImageView& getTextureImageView(const char* name);

private:
	VkContext* context;

	// dedicated container for material textures i.e. grouped
	std::unordered_map<Util::String, std::vector<MaterialTextureInfo>> groupedTextures;

	// single textures derived from the asset manager
	std::unordered_map<Util::String, TextureInfo> textures;

	// a queue of descriptor sets which need updating on a per frame basis - for single textures
	std::vector<DescrSetUpdateInfo> descriptorSetUpdateQueue;

	// associate textures with descriptor layouts
	std::unordered_map<const char*, TextureLayoutInfo> textureLayouts;
};

}    // namespace VulkanAPI
