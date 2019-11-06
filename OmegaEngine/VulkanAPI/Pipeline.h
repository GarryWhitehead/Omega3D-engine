#pragma once
#include "Rendering/RenderableTypes/RenderableBase.h"
#include "VulkanAPI/Common.h"
#include "VulkanAPI/Renderpass.h"
#include "VulkanAPI/Shader.h"

namespace OmegaEngine
{
enum class StateTopology;
}

namespace VulkanAPI
{
// forward declearions
class Shader;
class RenderPass;
class PipelineLayout;

enum class PipelineType
{
	Graphics,
	Compute
};

class Pipeline
{
public:
	Pipeline();
	~Pipeline();

	void addVertexInput(uint32_t location, vk::Format format, uint32_t size);
	void updateVertexInput();

	void setRasterCullMode(vk::CullModeFlags cull_mode);
	void setRasterFrontFace(vk::FrontFace front_face);
	void setRasterDepthClamp(bool state);

	void setTopology(const OmegaEngine::StateTopology &topology);

	void addColourAttachment(bool blend_factor, RenderPass &renderpass);
	void addDynamicState(vk::DynamicState state);

	void setStencilStateFrontAndBack(vk::CompareOp compareOp, vk::StencilOp failOp,
	                                 vk::StencilOp depthFailOp, vk::StencilOp passOp,
	                                 uint32_t compareMask, uint32_t writeMask, uint32_t ref);

	void setDepthState(bool test_state, bool write_state,
	                   vk::CompareOp compare = vk::CompareOp::eLessOrEqual);
	void setRenderpass(RenderPass &pass);
	void addShader(Shader &shader);
	void addLayout(vk::PipelineLayout &layout);
	void addEmptyLayout();

	void create(vk::Device dev, RenderPass &renderpass, Shader &shader, PipelineLayout &layout,
	            PipelineType _type);
	void create(vk::Device dev, RenderPass &renderpass, Shader &shader, PipelineType _type);
	void create(vk::Device dev, PipelineType _type);

	static void reflect(uint32_t* data, size_t dataSize, Pipeline& pipeline);
	
	PipelineType getPipelineType() const
	{
		return type;
	}

	vk::Pipeline &get()
	{
		return pipeline;
	}

private:
	vk::Device device;

	// everything needeed to build the pipeline
	std::vector<vk::VertexInputAttributeDescription> vertexAttrDescr;
	std::vector<vk::VertexInputBindingDescription> vertexBindDescr;
	vk::PipelineVertexInputStateCreateInfo vertexInputState;
	vk::PipelineInputAssemblyStateCreateInfo assemblyState;
	vk::PipelineViewportStateCreateInfo viewportState;
	vk::PipelineRasterizationStateCreateInfo rasterState;
	vk::PipelineMultisampleStateCreateInfo multiSampleState;
	std::vector<vk::PipelineColorBlendAttachmentState> colorAttachState;
	vk::PipelineColorBlendStateCreateInfo colorBlendState;
	vk::PipelineDepthStencilStateCreateInfo depthStencilState;
	std::vector<vk::DynamicState> dynamicStates;
	vk::PipelineDynamicStateCreateInfo dynamicCreateState;

	Shader shader;
	RenderPass renderpass;
	vk::PipelineLayout pipelineLayout;
	vk::Pipeline pipeline;

	PipelineType type;
};

// all the data required to create a pipeline layout
class PipelineLayout
{
public:
	PipelineLayout();

	void create(vk::Device &device,
	            std::vector<std::tuple<uint32_t, vk::DescriptorSetLayout>> &descriptorLayout);

	vk::PipelineLayout &get()
	{
		return layout;
	}

	void add_push_constant(StageType stage, uint32_t size)
	{
		pushConstantSizes[(int)stage] = size;
	}

	static void reflect(uint32_t* data, size_t dataSize, PipelineLayout& layout);

private:
	// usually set through shader reflection
	std::array<uint32_t, (int)StageType::Count> pushConstantSizes = {};

	vk::PipelineLayout layout;
};

} // namespace VulkanAPI
