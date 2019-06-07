#pragma once

#include "tiny_gltf.h"

namespace OmegaEngine
{
	enum class TextureFormat
	{
		Image8UC4,
		Image16UC4,
		ImageBC3
	};

	struct Sampler
	{
		Sampler(vk::SamplerAddressMode _mode, vk::Filter _filter) :
			mode(_mode),
			filter(_filter)
		{}

		vk::SamplerAddressMode mode;
		vk::Filter filter;
	};

	struct Texture
	{
		Texture() :
			name(_name)
		{}

		

		
	};


	class ModelImage
	{

	public:

		


		ModelImage(std::string& _name);
		~ModelImage();

		void map(uint32_t width, uint32_t height, void* data)
		{
			assert(data != nullptr);
			uint32_t size = width * height * 4;
			memcpy(imageData, data, size);
			assert(imageData != nullptr);

			// images must be 4-channel RGBA
			format = TextureFormat::Image8UC4;
		}

	private:

		std::string name;

		uint8_t* imageData;
		uint32_t width;
		uint32_t height;

		// usually 4 channel RGBA
		TextureFormat format;

		// nullptr if not specfied in the gltf file
		std::unique_ptr<Sampler> sampler;
	};

}
