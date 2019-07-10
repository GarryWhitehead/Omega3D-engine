#include "ModelImage.h"
#include "Utility/Logger.h"

namespace OmegaEngine
{

ModelImage::ModelImage()
{
}

ModelImage::ModelImage(std::string _name)
    : name(_name)
{
}

ModelImage::~ModelImage()
{
	if (imageData)
	{
		delete[] imageData;
		imageData = nullptr;
	}
}

void ModelSampler::getWrapMode(int32_t wrap)
{
	switch (wrap)
	{
	case 10497:
		mode = vk::SamplerAddressMode::eRepeat;
		break;
	case 33071:
		mode = vk::SamplerAddressMode::eClampToEdge;
		break;
	case 33648:
		mode = vk::SamplerAddressMode::eMirroredRepeat;
		break;
	default:
		LOGGER_INFO("Unsupported wrap mode %i whilst parsing gltf sampler.", wrap);
		mode = vk::SamplerAddressMode::eClampToBorder;
	}
}

void ModelSampler::getFilterMode(int32_t filter)
{
	switch (filter)
	{
	case 9728:
		this->filter = vk::Filter::eNearest;
		break;
	case 9729:
		this->filter = vk::Filter::eLinear;
		break;
	case 9984:
		this->filter = vk::Filter::eNearest;
		break;
	case 9986:
		this->filter = vk::Filter::eLinear;
		break;
	case 9987:
		this->filter = vk::Filter::eLinear;
		break;
	default:
		LOGGER_INFO("Unsupported filter mode %i whilst parsing gltf sampler.", filter);
		this->filter = vk::Filter::eNearest;
	}
}

void ModelImage::map(uint32_t width, uint32_t height, void *data)
{
	assert(data != nullptr);
	uint32_t size = width * height * 4;
	imageData = new uint8_t[size];
	memcpy(imageData, data, size);
	assert(imageData != nullptr);

	this->width = width;
	this->height = height;

	// images must be 4-channel RGBA
	format = TextureFormat::Image8UC4;
}

} // namespace OmegaEngine
