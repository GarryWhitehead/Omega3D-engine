#pragma once

#include <vector>
#include "Vulkan/Common.h"
#include "tiny_gltf.h"

namespace OmegaEngine
{
	// forward declerations
	class MappedTexture;

	class TextureManager
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

		void addGltfSampler(tinygltf::Sampler& sampler);
		void addGltfImage(tinygltf::Image& image);

		vk::SamplerAddressMode get_wrap_mode(int32_t wrap);
		vk::Filter get_filter_mode(int32_t filter);

		MappedTexture& get_texture(uint32_t index);

	private:

		std::vector<MappedTexture> textures;
		std::vector<Sampler> samplers;
	};

}

