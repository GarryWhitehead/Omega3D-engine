#pragma once
#include "Vulkan/Common.h"

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

	class ModelImage
	{

	public:

		ModelImage(std::string _name);
		~ModelImage();

		void extractfImageData(tinygltf::Model& model, tinygltf::Texture& texture);

		static vk::SamplerAddressMode getWrapMode(int32_t wrap);
		static vk::Filter getFilterMode(int32_t filter);

		void map(uint32_t width, uint32_t height, void* data)
		{
			assert(data != nullptr);
			uint32_t size = width * height * 4;
			//memcpy(imageData, data, size);
			assert(imageData != nullptr);

			// images must be 4-channel RGBA
			format = TextureFormat::Image8UC4;
		}

		uint32_t getWidth() const
		{
			return width;
		}

		uint32_t getHeight() const
		{
			return height;
		}

		uint8_t* getData() 
		{
			return imageData;
		}

		TextureFormat getFormat() const
		{
			return format;
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
