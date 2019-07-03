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

vk::SamplerAddressMode ModelImage::getWrapMode(int32_t wrap)
{
	vk::SamplerAddressMode ret;

	switch (wrap)
	{
	case 10497:
		ret = vk::SamplerAddressMode::eRepeat;
		break;
	case 33071:
		ret = vk::SamplerAddressMode::eClampToEdge;
		break;
	case 33648:
		ret = vk::SamplerAddressMode::eMirroredRepeat;
		break;
	default:
		LOGGER_INFO("Unsupported wrap mode %i whilst parsing gltf sampler.", wrap);
		ret = vk::SamplerAddressMode::eClampToBorder;
	}
	return ret;
}

vk::Filter ModelImage::getFilterMode(int32_t filter)
{
	vk::Filter ret;

	switch (filter)
	{
	case 9728:
		ret = vk::Filter::eNearest;
		break;
	case 9729:
		ret = vk::Filter::eLinear;
		break;
	case 9984:
		ret = vk::Filter::eNearest;
		break;
	case 9986:
		ret = vk::Filter::eLinear;
		break;
	case 9987:
		ret = vk::Filter::eLinear;
		break;
	default:
		LOGGER_INFO("Unsupported filter mode %i whilst parsing gltf sampler.", filter);
		ret = vk::Filter::eNearest;
	}
	return ret;
}

void ModelImage::extractfImageData(tinygltf::Model &model, tinygltf::Texture &texture)
{
	// map to temporary storage until transferred to the asset manager
	tinygltf::Image image = model.images[texture.source];
	map(image.width, image.height, image.image.data());

	// nor guarenteed to have a sampler
	if (texture.sampler > -1)
	{
		tinygltf::Sampler gltfSampler = model.samplers[texture.sampler];

		vk::SamplerAddressMode mode = getWrapMode(gltfSampler.wrapS);
		vk::Filter filter = getFilterMode(gltfSampler.minFilter);

		sampler = std::make_unique<Sampler>(mode, filter);
	}
}
} // namespace OmegaEngine
