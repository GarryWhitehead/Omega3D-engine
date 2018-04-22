#pragma once
#include "VulkanCore/VulkanModule.h"
#include "VulkanCore/ColladaModelInfo.h"

class VulkanEngine;
class ModelResourceManager;
class CameraSystem;

const int MAX_BONES = 64;

class VulkanAnimation : public VulkanModule
{

public:

	struct AnimationSets
	{
		VulkanUtility::DescriptorInfo descriptors;
	};

	struct AnimationInfo
	{
		AnimationSets mesh;
		AnimationSets material;

		VkPipelineLayout pipelineLayout;
		struct MaterialPipelines
		{
			VulkanUtility::PipeLlineInfo diffNorm;
			VulkanUtility::PipeLlineInfo diff;
			VulkanUtility::PipeLlineInfo nomap;
		} matPipelines;

		struct MaterialDeccriptors
		{
			VkDescriptorSetLayout diffLayout;
			VkDescriptorSetLayout nomapLayout;
			VkDescriptorSetLayout diffnormLayout;
		} descriptors;

		std::array<VkPipelineShaderStageCreateInfo, 2> shader;
	};

	struct UboLayout
	{
		glm::mat4 projection;
		glm::mat4 viewMatrix;
		glm::mat4 modelMatrix;
		glm::mat4 boneTransform[MAX_BONES];
	};

	VulkanAnimation(VulkanEngine *engine, VulkanUtility *utility);
	~VulkanAnimation();

	void Destroy() override;
	void PrepareMeshDescriptorSet();
	void PrepareMaterialDescriptorPool(uint32_t materialCount);
	void PrepareMaterialDescriptorLayouts();
	void PrepareMaterialDescriptorSets(ColladaModelInfo::Material *material);
	void PreparePipeline();
	void GenerateModelCmdBuffer(VkCommandBuffer cmdBuffer, VkDescriptorSet set, VkPipelineLayout layout, VkPipeline pipeline);
	void CreateVertexBuffer(uint32_t bufferSize);
	void CreateIndexBuffer(uint32_t bufferSize);
	void PrepareUBOBuffer();
	void MapDataToBuffers(ColladaModelInfo *model, uint32_t vertexOffset, uint32_t indexOffset);
	void Update(CameraSystem *camera);

	friend class VulkanEngine;
	friend class ModelResourceManager;

private:

	VulkanEngine *p_vkEngine;
	ModelResourceManager *p_modelManager;

	// buffer info for the "mega" buffer which holds all the models and are referenced via offsets
	BufferData m_vertexBuffer;
	BufferData m_indexBuffer;
	BufferData m_uboBuffer;

	AnimationInfo m_animInfo;

};
