#pragma once
#include "VulkanAPI/Common.h"

#include <memory>

namespace OmegaEngine
{
enum class TextureFormat
{
	Image8UC4,
	Image16UC4,
	ImageBC3
};

struct ModelSampler
{

	ModelSampler() = default;
	
	ModelSampler(const int32_t mode, const int32_t filter)
	{
		getWrapMode(mode);
		getFilterMode(filter);
	}

	~ModelSampler()
	{
	}

	void getWrapMode(int32_t wrap);
	void getFilterMode(int32_t filter);

	vk::SamplerAddressMode mode;
	vk::Filter filter;
};

class ModelImage
{
public:

	ModelImage();
	ModelImage(std::string _name);
	~ModelImage();

	void map(uint32_t width, uint32_t height, void *data);
	
	uint32_t getWidth() const
	{
		return width;
	}

	uint32_t getHeight() const
	{
		return height;
	}

	uint8_t *getData()
	{
		return imageData;
	}

	TextureFormat getFormat() const
	{
		return format;
	}
	
	void addSampler(const int wrapS, const int mode)
	{
		sampler = std::make_unique<ModelSampler>(wrapS, mode);
	}

	std::unique_ptr<ModelSampler> &getSampler()
	{
		return sampler;
	}

	friend class MaterialInfo;

private:
	std::string name;

	uint8_t *imageData = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;

	// usually 4 channel RGBA
	TextureFormat format;

	// nullptr if not specfied in the gltf file
	std::unique_ptr<ModelSampler> sampler;
};

} // namespace OmegaEngine
