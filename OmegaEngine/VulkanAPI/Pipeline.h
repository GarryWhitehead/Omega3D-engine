#pragma once

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
class PipelineLayout;
class VkContext;

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

    // a reference to the shader associated with this pipline
	Shader& shader;
    
    // a reference to the renderpass associated with this pipeline
	RenderPass& renderpass;
    
    // a reference to the layout associated with this pipeline
	vk::PipelineLayout& pipelineLayout;
    
	vk::Pipeline pipeline;

	PipelineType type;
};

/**
* @brief all the data required to create a pipeline layout
*/
class PipelineLayout
{
public:
	PipelineLayout();
    
    /**
     * @brief Creates a pipleine layout based on the push constants and descriptor layout which must be
     * created before calling this function.
     * Note: The layouts must be in the correct order as depicted by the set number
     */
	void prepare(VkContext& context, std::vector<DescriptorLayout> &descrLayouts);
    
    /// returns the vulkan pipeline layout
	vk::PipelineLayout &get()
	{
		return layout;
	}
    
    /**
     * Adds a push constant reference to this layout. Only the size must be known at this stage; The data will be
     * associated with this push constant at draw time.
     */
	void addPushConstant(Shader::StageType stage, uint32_t size)
	{
        pConstantSizes.emplace(stage, size);
	}

private:
    
	/// the shader stage the push constant refers to and its size
	std::vector<std::pair<Shader::StageType, size_t> pConstantSizes = {};

	vk::PipelineLayout layout;
};

} // namespace VulkanAPI
