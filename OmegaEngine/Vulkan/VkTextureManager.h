#pragma once
#include "Vulkan/Common.h"
#include "Managers/EventManager.h"
#include "Managers/DataTypes/TextureType.h"
#include "Vulkan/Sampler.h"
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
		TextureUpdateEvent(const char* _id, OmegaEngine::MappedTexture* _mapped, SamplerType _sampler) :
			id(_id),
			mapped_tex(_mapped),
			sampler(_sampler)
		{}
		
		const char* id;
		uint32_t binding = 0;
		OmegaEngine::MappedTexture* mapped_tex;
		SamplerType sampler;
	};


	class VkTextureManager
	{

	public:
		
		struct TextureInfo
		{
			Texture texture;
			Sampler sampler;
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

		void bind_textures_to_layout(const char* id, DescriptorLayout* layout);

	private:

		vk::Device device;
		vk::PhysicalDevice gpu;
		VulkanAPI::Queue graph_queue;

		std::unordered_map<const char*, std::vector<TextureInfo> > textures;

		// a queue of descriptor sets which need updating this frame
		std::vector<DescrSetUpdateInfo> descr_set_update_queue;

		std::unordered_map<const char*, DescriptorLayout*> texture_layouts;
	};

}

