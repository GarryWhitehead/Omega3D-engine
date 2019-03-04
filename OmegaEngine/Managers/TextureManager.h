#pragma once

#include "Managers/ManagerBase.h"
#include "Vulkan/Common.h"

#include "tiny_gltf.h"

#include <vector>

namespace VulkanAPI
{
	enum class SamplerType;
}

namespace OmegaEngine
{
	// forward declerations
	class MappedTexture;

	class TextureManager : public ManagerBase
	{
	public:


		struct Sampler
		{
			vk::SamplerAddressMode addressModeU;
			vk::SamplerAddressMode addressModeV;
			vk::SamplerAddressMode addressModeW;
			vk::Filter minFilter;
			vk::Filter magFilter;
		};

		TextureManager();
		~TextureManager();

		void addGltfSampler(tinygltf::Sampler& sampler);
		void addGltfImage(tinygltf::Image& image);

		vk::SamplerAddressMode get_wrap_mode(int32_t wrap);
		vk::Filter get_filter_mode(int32_t filter);

		uint32_t get_texture_index(uint32_t set, const char* name);
		MappedTexture& get_texture(uint32_t set, int index);
		VulkanAPI::SamplerType get_sampler(uint32_t index);

		void next_set()
		{
			++current_set;
		}

	private:

		// keep a record of the current texture set
		uint32_t current_set = 0;

		std::unordered_map<uint32_t, std::vector<MappedTexture> > textures;
		std::vector<VulkanAPI::SamplerType> samplers;
	};

}

