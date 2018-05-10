#pragma once
#include "VulkanCore/VulkanModule.h"
#include "glm.hpp"
#include <gtc/matrix_transform.hpp>

class VulkanEngine;
class CameraSystem;

class VulkanIBL : public VulkanModule
{
public:

	const int PREFILTERMAP_DIM = 512;
	const int IRRADIANCEMAP_DIM = 64;			// 64 x 64 samples
	const int MIP_LEVELS = 5;

	const float PI = 3.1415926535897932384626433832795f;

	// view from each of the cubemap faces
	//const glm::mat4 cubeView[6] =
	//{
	//	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),			// positive X
	//	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),		// negative X
	//	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),			// positive Y
	//	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),		// negative Y
	//	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),			// positive Z
	//	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f))			// negative Z
	//};

	std::vector<glm::mat4> cubeView = {
		// POSITIVE_X
		glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// NEGATIVE_X
		glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// POSITIVE_Y
		glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// NEGATIVE_Y
		glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// POSITIVE_Z
		glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// NEGATIVE_Z
		glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
	};

	struct FilterPushConstant
	{
		glm::mat4 mvp;				// offset = 0
		float roughness;			// offset = 64
		uint32_t sampleCount;		// offset = 68
	};

	struct IBLInfo
	{
		TextureInfo cubeImage;
		TextureInfo offscreenImage;
		VkFramebuffer offscreenFB;
		VkRenderPass renderpass;
		VkPipeline pipeline;
	};

	VulkanIBL(VulkanEngine *engine, VulkanUtility *utility);
	~VulkanIBL();

	void Init();
	void Update(CameraSystem *camera);
	void Destroy() override;
	void LoadAssets();
	void PrepareImage(VkFormat format, TextureInfo& imageInfo, int dim, int mipLevels);
	void PrepareRenderpass(VkFormat format, VkRenderPass& renderpass);
	void PrepareIBLDescriptors();
	void PrepareOffscreenFrameBuffer(VkFormat format, TextureInfo& imageInfo, VkRenderPass renderpass, int dim, VkFramebuffer& framebuffer);
	void PrepareIBLPipeline();
	void GenerateIrrMapCmdBuffer();
	void GeneratePreFilterCmdBuffer();

	friend class VulkanDeferred;
	friend class VulkanSkybox;

private:

	VulkanEngine * p_vkEngine;

	std::array<VkPipelineShaderStageCreateInfo, 2> m_shader;

	VulkanUtility::DescriptorInfo m_descriptors;

	TextureInfo m_cubeImage;
	VkPipelineLayout m_pipelineLayout;

	IBLInfo m_irradianceCube;
	IBLInfo m_filterCube;

};

