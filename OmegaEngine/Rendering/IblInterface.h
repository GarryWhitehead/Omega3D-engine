#pragma once

#include "Vulkan/Common.h"
#include "Vulkan/DataTypes/Texture.h"
#include "OEMaths/OEMaths.h"

#include <vector>

namespace VulkanAPI
{
	class Queue;
	class Interface;
}

namespace OmegaEngine
{
	class RenderConfig;

	class IblInterface
	{

	public:

		const uint32_t irradianceMapDim = 64;
		const uint8_t mipLevels = 5;

		std::vector<OEMaths::mat4f> cubeView = {
			// POSITIVE_X
			OEMaths::mat4f::rotate(180.0f, OEMaths::vec3f(1.0f, 0.0f, 0.0f)) * OEMaths::mat4f::rotate(90.0f, OEMaths::vec3f(0.0f, 1.0f, 0.0f)),
			// NEGATIVE_X
			OEMaths::mat4f::rotate(180.0f, OEMaths::vec3f(1.0f, 0.0f, 0.0f)) * OEMaths::mat4f::rotate(-90.0f, OEMaths::vec3f(0.0f, 1.0f, 0.0f)),
			// POSITIVE_Y
			OEMaths::mat4f::rotate(90.0f, OEMaths::vec3f(1.0f, 0.0f, 0.0f)),
			// NEGATIVE_Y
			OEMaths::mat4f::rotate(90.0f, OEMaths::vec3f(1.0f, 0.0f, 0.0f)),
			// POSITIVE_Z
			OEMaths::mat4f::rotate(180.0f, OEMaths::vec3f(1.0f, 0.0f, 0.0f)),
			// NEGATIVE_Z
			OEMaths::mat4f::rotate(180.0f, OEMaths::vec3f(0.0f, 0.0f, 1.0f))
		};

		// push constant layout for irradiance map cube
		struct IrradianceMapPushBlock
		{
			OEMaths::mat4f mvp;			// offset = 0
			float roughness;			// offset = 64
			uint32_t sampleCount;		// offset = 68
		};

		IblInterface(VulkanAPI::Interface& vkInterface);
		~IblInterface();

		void generateBrdf(VulkanAPI::Interface& vkInterface);
		void createIrradianceMap(VulkanAPI::Interface& vkInterface);
		void renderMaps(VulkanAPI::Interface& vkInterface);

		vk::ImageView& getBrdfImageView()
		{
			return brdfTexture.getImageView();
		}

		vk::ImageView& getIrradianceMapImageView()
		{
			return irradianceMapTexture.getImageView();
		}

		bool isReady() const
		{
			return mapsRendered;
		}

	private:

		VulkanAPI::Texture brdfTexture;
		VulkanAPI::Texture irradianceMapTexture;

		// if the images have been created from scratch, save to disc on destruction
		bool saveOnDestroy = false;

		// states whether the maps have been created
		bool mapsRendered = false;
	};

}


