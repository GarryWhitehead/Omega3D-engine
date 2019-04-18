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

	struct TextureUpdateEvent : public OmegaEngine::Event
	{
		TextureUpdateEvent(std::string _id, uint32_t _binding, OmegaEngine::MappedTexture* _mapped, SamplerType _sampler) :
			id(_id),
			binding(_binding),
			mapped_tex(_mapped),
			sampler(_sampler)
		{
		}
		
		//TextureUpdateEvent() {}
		
		std::string id;
		uint32_t binding = 0;
		OmegaEngine::MappedTexture* mapped_tex = nullptr;
		SamplerType sampler;
	};


	class VkTextureManager
	{

	public:
		
		struct TextureInfo
		{
			Texture texture;
			Sampler sampler;
			uint32_t binding = 0;
		};

		struct TextureLayoutInfo
		{
			DescriptorLayout* layout = nullptr;
			uint32_t set_num;
		};

		struct DescrSetUpdateInfo
		{
			const char *id;
			DescriptorSet* set = nullptr;
			uint32_t set_num = 0;
			uint32_t binding = 0;
		};

		VkTextureManager(vk::Device& dev, vk::PhysicalDevice& phys_dev, VulkanAPI::Queue& queue);
		~VkTextureManager();

		void update_texture(TextureUpdateEvent& event);
		void update_descriptors();

		// associates an id with a descriptor layout. Used for materials, etc. were there are multiple descriptor sets but one layout
		void bind_textures_to_layout(const char* id, DescriptorLayout* layout, uint32_t set_num);

		// updates a single descriptor set with a texture set identified by its unique id
		void update_descr_set(DescriptorSet& set, const char* id, uint32_t set_num);

		TextureLayoutInfo get_texture_descr_layout(const char* id);

	private:

		vk::Device device;
		vk::PhysicalDevice gpu;
		VulkanAPI::Queue graph_queue;

		// textures - can be grouped (i.e. materials) or single textures
		std::unordered_map<const char*, std::vector<TextureInfo> > textures;

		// a queue of descriptor sets which need updating on a per frame basis - not used yet, maybe remove?
		std::vector<DescrSetUpdateInfo> descr_set_update_queue;

		// associate textures with descriptor layouts
		std::unordered_map<const char*, TextureLayoutInfo> texture_layouts;
	};

}

