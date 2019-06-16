#pragma once
#include "VulkanAPI/Common.h"
#include "Managers/EventManager.h"
#include "AssetInterface/MappedTexture.h"
#include "AssetInterface/AssetManager.h"
#include "VulkanAPI/Sampler.h"
#include "VulkanAPI/DataTypes/Texture.h"
#include <unordered_map>
#include <tuple>

namespace VulkanAPI
{
	// forward declerations
	class DescriptorSet;
	class DescriptorLayout;
	enum class TextureType;
	class Texture;
	class Queue;

	struct MaterialTextureUpdateEvent : public OmegaEngine::Event
	{
		MaterialTextureUpdateEvent(std::string _id, uint32_t _binding, OmegaEngine::MappedTexture* _mapped, SamplerType _sampler) :
			id(_id),
			binding(_binding),
			mappedTexture(_mapped),
			sampler(_sampler)
		{
		}
			
		std::string id;
		uint32_t binding = 0;
		OmegaEngine::MappedTexture* mappedTexture = nullptr;
		SamplerType sampler;
	};

	struct TextureUpdateEvent : public OmegaEngine::Event
	{
		TextureUpdateEvent(std::string _id, OmegaEngine::AssetManager::TextureAssetInfo* info) :
			id(_id),
			textureInfo(info)
		{
		}

		std::string id;
		OmegaEngine::AssetManager::TextureAssetInfo* textureInfo = nullptr;
		Sampler sampler;
	};

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
			const char *id;
			DescriptorSet* set = nullptr;
			Sampler* sampler = nullptr;
			uint32_t setValue = 0;
			uint32_t binding = 0;
		};

		VkTextureManager(vk::Device& dev, vk::PhysicalDevice& physicalDevice, VulkanAPI::Queue& queue);
		~VkTextureManager();

		void updateTexture(TextureUpdateEvent& event);
		void enqueueDescrUpdate(const char*, VulkanAPI::DescriptorSet*, VulkanAPI::Sampler* sampler, uint32_t set, uint32_t binding);
		void updateDescriptors();
		void update();

		// updates a single descriptor set with a texture set identified by its unique id
		void updateMaterialDescriptorSet(DescriptorSet& set, const char* id, uint32_t setValue);

		void updateMaterialTexture(MaterialTextureUpdateEvent& event);
		void updateMaterialDescriptors();

		// associates an id with a descriptor layout. Used for materials, etc. were there are multiple descriptor sets but one layout
		void bindTexturesToDescriptorLayout(const char* id, DescriptorLayout* layout, uint32_t setValue);

		TextureLayoutInfo& getTextureDescriptorLayout(const char* id);

		vk::ImageView& getTextureImageView(const char* name);

	private:

		vk::Device device;
		vk::PhysicalDevice gpu;
		VulkanAPI::Queue graphicsQueue;

		// dedicated container for material textures i.e. grouped
		std::unordered_map<std::string, std::vector<MaterialTextureInfo> > materialTextures;

		// single textures derived from the asset manager
		std::unordered_map<const char*, TextureInfo> textures;

		// a queue of descriptor sets which need updating on a per frame basis - for single textures
		std::vector<DescrSetUpdateInfo> descriptorSetUpdateQueue;

		// associate textures with descriptor layouts
		std::unordered_map<const char*, TextureLayoutInfo> textureLayouts;
	};

}

