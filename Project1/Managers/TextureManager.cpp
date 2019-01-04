#include "TextureManager.h"

#include "DataTypes/TextureType.h"
#include "stb_image.h"
#include "Utility/logger.h"

namespace OmegaEngine
{

	TextureManager::TextureManager()
	{
	}


	TextureManager::~TextureManager()
	{
	}

	vk::SamplerAddressMode TextureManager::get_wrap_mode(int32_t wrap)
	{
		vk::SamplerAddressMode ret;

		switch (wrap) {
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

	vk::Filter TextureManager::get_filter_mode(int32_t filter)
	{
		vk::Filter ret;

		switch (filter) {
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

	void TextureManager::addGltfSampler(tinygltf::Sampler& gltf_sampler)
	{
		Sampler sampler;
		sampler.addressModeU = get_wrap_mode(gltf_sampler.wrapS);
		sampler.addressModeV = get_wrap_mode(gltf_sampler.wrapT);
		sampler.addressModeV = sampler.addressModeV;
		sampler.minFilter = get_filter_mode(gltf_sampler.minFilter);
		sampler.magFilter = get_filter_mode(gltf_sampler.magFilter);
		samplers.push_back(sampler);
	}

	void TextureManager::addGltfImage(tinygltf::Image& image)
	{
		MappedTexture mappedTex;
		int imageSize = image.width * image.height;
		mappedTex.set_name(image.name.c_str());

		mappedTex.loadPngTexture(imageSize, image.image.data());
		textures.push_back(mappedTex);	
	}

	MappedTexture& TextureManager::get_texture(uint32_t index)
	{
		assert(index > textures.size());
		return textures[index];
	}

}