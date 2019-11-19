#pragma once

#include "OEMaths/OEMaths.h"

#include "VulkanAPI/Common.h"
#include "VulkanAPI/VkTexture.h"

#include <vector>

namespace VulkanAPI
{
class Queue;
class Interface;
}    // namespace VulkanAPI

namespace OmegaEngine
{
struct RenderConfig;

class IblInterface
{

public:
	const uint32_t irradianceMapDim = 64;
	const uint32_t specularMapDim = 512;
	const uint8_t mipLevels = 5;

	std::vector<OEMaths::mat4f> cubeView = {
		// POSITIVE_X
		OEMaths::mat4f::rotate(90.0f, OEMaths::vec3f(0.0f, 1.0f, 0.0f)) *
		    OEMaths::mat4f::rotate(180.0f, OEMaths::vec3f(1.0f, 0.0f, 0.0f)),
		// NEGATIVE_X
		OEMaths::mat4f::rotate(-90.0f, OEMaths::vec3f(0.0f, 1.0f, 0.0f)) *
		    OEMaths::mat4f::rotate(180.0f, OEMaths::vec3f(1.0f, 0.0f, 0.0f)),
		// POSITIVE_Y
		OEMaths::mat4f::rotate(-90.0f, OEMaths::vec3f(1.0f, 0.0f, 0.0f)),
		// NEGATIVE_Y
		OEMaths::mat4f::rotate(90.0f, OEMaths::vec3f(1.0f, 0.0f, 0.0f)),
		// POSITIVE_Z
		OEMaths::mat4f::rotate(180.0f, OEMaths::vec3f(1.0f, 0.0f, 0.0f)),
		// NEGATIVE_Z
		OEMaths::mat4f::rotate(180.0f, OEMaths::vec3f(0.0f, 0.0f, 1.0f))
	};

	// push constant layout for irradiance map cube
	struct SpecularMapPushBlock
	{
		OEMaths::mat4f mvp;
		float roughness;
		uint32_t sampleCount;
	};

	IblInterface(VulkanAPI::Interface& vkInterface);
	~IblInterface();

	void calculateCubeTransform(const uint32_t face, const float zNear, const float zFar, OEMaths::mat4f& outputProj,
	                            OEMaths::mat4f& outputView);
	void generateBrdf(VulkanAPI::Interface& vkInterface);
	void createIrradianceMap(VulkanAPI::Interface& vkInterface);
	void createSpecularMap(VulkanAPI::Interface& vkInterface);
	void renderMaps(VulkanAPI::Interface& vkInterface);

	vk::ImageView& getBrdfImageView()
	{
		return brdfTexture.getImageView();
	}

	vk::ImageView& getIrradianceMapImageView()
	{
		return irradianceMapTexture.getImageView();
	}

	vk::ImageView& getSpecularMapImageView()
	{
		return specularMapTexture.getImageView();
	}

	bool isReady() const
	{
		return mapsRendered;
	}

private:
	VulkanAPI::Texture brdfTexture;
	VulkanAPI::Texture irradianceMapTexture;
	VulkanAPI::Texture specularMapTexture;

	// if the images have been created from scratch, save to disc on destruction
	bool saveOnDestroy = false;

	// states whether the maps have been created
	bool mapsRendered = false;
};

}    // namespace OmegaEngine
