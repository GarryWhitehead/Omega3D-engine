#pragma once

#include "VulkanAPI/Common.h"
#include "VulkanAPI/Renderpass.h"
#include "VulkanAPI/Shader.h"

namespace VulkanAPI
{
// forward declearions
class Shader;
class PipelineLayout;
class VkDriver;

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

    // not copyable
    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    
	void updateVertexInput();

	void addEmptyLayout();
    
    /**
     * Creates a pipeline using render data from the shader program and associates it with the declared renderpass
     */
	void create(vk::Device dev, RenderPass &renderpass, ShaderProgram &shader);
	
	PipelineType getType() const
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

    PipelineType type;
    
    // a reference to the shader associated with this pipline
	Shader& shader;
    
    // a reference to the renderpass associated with this pipeline
	RenderPass& renderpass;
    
    // a reference to the layout associated with this pipeline
	PipelineLayout& pipelineLayout;
    
	vk::Pipeline pipeline;
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
	void prepare(VkDriver& context, std::vector<DescriptorLayout> &descrLayouts);
    
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
