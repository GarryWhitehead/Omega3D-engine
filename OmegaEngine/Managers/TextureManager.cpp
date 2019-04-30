#include "TextureManager.h"
#include "DataTypes/TextureType.h"
#include "Utility/logger.h"
#include "Vulkan/Sampler.h"
#include "Omega_Common.h"
#include "Objects/ObjectManager.h"
#include "Managers/ComponentInterface.h"

namespace OmegaEngine
{

	TextureManager::TextureManager()
	{
		// this won't be used, just required to keep vulkan happy
		dummy_texture.create_empty_texture(1024, 1024, true);
	}


	TextureManager::~TextureManager()
	{
	}

	void TextureManager::update_frame(double time, double dt,
		std::unique_ptr<ObjectManager>& obj_manager,
		ComponentInterface* component_manager)
	{

	}

	vk::SamplerAddressMode TextureManager::get_wrap_mode(int32_t wrap)
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

	vk::Filter TextureManager::get_filter_mode(int32_t filter)
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

	void TextureManager::addGltfSampler(uint32_t set, tinygltf::Sampler& gltf_sampler)
	{
		VulkanAPI::SamplerType type;
		vk::SamplerAddressMode mode = get_wrap_mode(gltf_sampler.wrapS);
		vk::Filter filter = get_filter_mode(gltf_sampler.minFilter);
		
		if (mode == vk::SamplerAddressMode::eRepeat && filter == vk::Filter::eLinear) 
		{
			type = VulkanAPI::SamplerType::LinearWrap;
		}
		if (mode == vk::SamplerAddressMode::eClampToEdge && filter == vk::Filter::eLinear) 
		{
			type = VulkanAPI::SamplerType::LinearClamp;
		}
		if (mode == vk::SamplerAddressMode::eClampToEdge && filter == vk::Filter::eNearest) 
		{
			type = VulkanAPI::SamplerType::Clamp;
		}
		if (mode == vk::SamplerAddressMode::eRepeat && filter == vk::Filter::eLinear) 
		{
			type = VulkanAPI::SamplerType::Wrap;
		}

		samplers[set].push_back(type);
	}

	void TextureManager::addGltfImage(tinygltf::Image& image)
	{
		MappedTexture mappedTex;
		mappedTex.set_name(image.uri.c_str());

		// probably should check for different types - though only 4 channels supported
		mappedTex.set_format(vk::Format::eR8G8B8A8Unorm); 

		if (!mappedTex.map_texture(image.width, image.height, image.component, image.image.data(), true)) 
		{
			// need to use a default texture here!
		}
		textures[current_set].push_back(mappedTex);	
	}

	uint32_t TextureManager::get_texture_index(uint32_t set, const char* name)
	{
		for (uint32_t i = 0; i < textures.size(); ++i) 
		{
			if (strcmp(name, textures[set][i].get_name()) == 0) 
			{
				return i;
			}
		}
		return UINT32_MAX;
	}

	MappedTexture& TextureManager::get_texture(uint32_t set, int index)
	{
		assert(index < textures[set].size());
		return textures[set][index];
	}

	VulkanAPI::SamplerType TextureManager::get_sampler(uint32_t set, uint32_t index)
	{
		// not all gltf files speciify sampler types so go with a default otherwise
		if (samplers[set].empty()) 
		{
			// if no samplers defined then go with this as default for now
			// TODO: Add option in shader reflection to add samplers based on image name
			return VulkanAPI::SamplerType::Clamp;	
		}
		return samplers[set][index];
	}

	VulkanAPI::SamplerType TextureManager::get_dummy_sampler()
	{
		// this could be anything as its not used
		return VulkanAPI::SamplerType::Clamp;
	}

}