#pragma once

#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/Common.h"
#include "VulkanAPI/Shader.h"

namespace VulkanAPI
{
// forward declearions
class Shader;
class PipelineLayout;
struct VkContext;
class ShaderProgram;
class RenderPass;
class DescriptorPool;

class Pipeline
{
public:
    
    enum class Type
    {
        Graphics,
        Compute
    };
    
	Pipeline(VkContext& context, RenderPass& rpass, PipelineLayout& layout, Pipeline::Type type);
	~Pipeline();

    // not copyable
    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    
    static vk::PipelineBindPoint createBindPoint(Pipeline::Type type);
    
	vk::PipelineVertexInputStateCreateInfo updateVertexInput(std::vector<ShaderProgram::InputBinding>& inputs);

	void addEmptyLayout();
    
    /**
     * Creates a pipeline using render data from the shader program and associates it with the declared renderpass
     */
	void create(ShaderProgram& shader);
	
	Pipeline::Type getType() const
	{
		return type;
	}

	vk::Pipeline &get()
	{
		return pipeline;
	}

private:
    
	VkContext& context;

	// everything needeed to build the pipeline
	std::vector<vk::VertexInputAttributeDescription> vertexAttrDescr;
	vk::VertexInputBindingDescription vertexBindDescr;
    
    // dynamic states to be used with this pipeline - by default the viewport and scissor dynamic states are set
    std::vector<vk::DynamicState> dynamicStates
    {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };
    
    Type type;
    
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
    void prepare(VkContext& context);
    
    /// returns the vulkan pipeline layout
    vk::PipelineLayout &get();
    
    /**
     * Adds a push constant reference to this layout. Only the size must be known at this stage; The data will be
     * associated with this push constant at draw time.
     */
    void addPushConstant(Shader::Type stage, uint32_t size);
    
    void addDescriptorLayout(uint8_t set, const vk::DescriptorSetLayout& layout);

private:
    
    // we store the layouts here for the descriptor as they may be added from multiple sources
    // i.e materials update from elsewhere. The set value is use to sort into the correct order
    std::vector<std::pair<uint8_t, vk::DescriptorSetLayout>> descriptorLayouts;
    
	// the shader stage the push constant refers to and its size
	std::vector<std::pair<Shader::Type, uint32_t>> pConstantSizes;
	vk::PipelineLayout layout;
};

} // namespace VulkanAPI
