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

		VkTextureManager(vk::Device& dev, vk::PhysicalDevice& phys_dev, VulkanAPI::Queue& queue);
		~VkTextureManager();

		void update_texture(TextureUpdateEvent& event);

		void update_descriptors();

	private:

		vk::Device device;
		vk::PhysicalDevice gpu;
		VulkanAPI::Queue graph_queue;

		std::unordered_map<const char*, TextureInfo> textures;

		// a queue of descriptor sets which need updating this frame
		std::vector<DescrSetUpdateInfo> descr_set_update_queue;
	};

}

