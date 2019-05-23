#pragma once
#include "Vulkan/Common.h"
#include "Managers/EventManager.h"
#include "Managers/DataTypes/TextureType.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/DataTypes/Texture.h"
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
		TextureUpdateEvent(std::string _id, OmegaEngine::MappedTexture* _mapped) :
			id(_id),
			mappedTexture(_mapped)
		{
		}

		std::string id;
		OmegaEngine::MappedTexture* mappedTexture = nullptr;
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

		void update_texture(TextureUpdateEvent& event);
		void enqueueDescrUpdate(const char*, VulkanAPI::DescriptorSet*, VulkanAPI::Sampler* sampler, uint32_t set, uint32_t binding);
		void updateDescriptors();
		void update();

		// updates a single descriptor set with a texture set identified by its unique id
		void update_material_descriptorSet(DescriptorSet& set, const char* id, uint32_t setValue);

		void update_material_texture(MaterialTextureUpdateEvent& event);
		void update_material_descriptors();

		// associates an id with a descriptor layout. Used for materials, etc. were there are multiple descriptor sets but one layout
		void bindTexturesToDescriptorLayout(const char* id, DescriptorLayout* layout, uint32_t setValue);

		TextureLayoutInfo& getTexture_descriptorLayout(const char* id);

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

