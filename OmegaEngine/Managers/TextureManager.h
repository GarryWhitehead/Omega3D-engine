#pragma once

#include "Managers/ManagerBase.h"
#include "Vulkan/Common.h"
#include "Managers/DataTypes/TextureType.h"

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

		// not used at present - just here to keep the inheritance demons happy
		void updateFrame(double time, double dt,
			std::unique_ptr<ObjectManager>& objectManager,
			ComponentInterface* componentInterface) override;

		void addGltfSampler(uint32_t set, tinygltf::Sampler& sampler);
		void addGltfImage(tinygltf::Image& image);

		vk::SamplerAddressMode getWrapMode(int32_t wrap);
		vk::Filter getFilterMode(int32_t filter);

		uint32_t getTextureIndex(uint32_t set, const char* name);
		MappedTexture& getTexture(uint32_t set, int index);
		VulkanAPI::SamplerType getSampler(uint32_t set, uint32_t index);
		VulkanAPI::SamplerType getDummySampler();

		void nextSet()
		{
			++currentSet;
		}

		uint32_t getCurrentSet() const
		{
			return currentSet;
		}

		MappedTexture& getDummyTexture()
		{
			return dummyTexture;
		}

		

	private:

		// keep a record of the current texture set
		uint32_t currentSet = 0;

		std::unordered_map<uint32_t, std::vector<MappedTexture> > textures;
		std::unordered_map<uint32_t, std::vector<VulkanAPI::SamplerType> > samplers;

		// dummy texture
		MappedTexture dummyTexture;
	};

}

