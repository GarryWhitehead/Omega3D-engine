#include "Pipeline.h"

#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/RenderPass.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/VkContext.h"

namespace VulkanAPI
{

PipelineLayout::PipelineLayout()
{
}

void PipelineLayout::prepare(VkContext& context,
                            std::vector<DescriptorLayout>& descrLayouts)
{
	// create push constants
	// TODO: this needs a bit of a refactor - only one pushcontant across stages allowed
	// - maybe this is OK, but if so, no need for a vector anymore
	std::vector<vk::PushConstantRange> pConstants;
	vk::ShaderStageFlags flags;
	uint32_t totalSize = 0;

	for (size_t i = 0; i < pConstantSizes.size(); ++i)
	{
        size_t size = pConstantSizes[i].first;
        Shader::Type type = pConstantSizes[i].second;
        
        flags |= Shader::getStageFlags(type);
        totalSize += size;
	}
    
	if (totalSize > 0)
	{
		vk::PushConstantRange push(flags, 0, totalSize);
		pConstants.push_back(push);
	}

	// create the layout - the descriptor layouts must be sorted in the correct order
	vk::PipelineLayoutCreateInfo pipelineInfo({}, static_cast<uint32_t>(descrLayouts.size()), descrLayouts.data(),
	                                          static_cast<uint32_t>(pConstants.size()), pConstants.data());

	VK_CHECK_RESULT(context.getDevice().createPipelineLayout(&pipelineInfo, nullptr, &layout));
}

// ================== pipeline =======================
Pipeline::Pipeline()
{
}

Pipeline::~Pipeline()
{
}

vk::PipelineBindPoint Pipeline::createBindPoint(Pipeline::Type type)
{
    vk::PipelineBindPoint bindPoint;

    switch (type)
    {
    case Type::Graphics:
        bindPoint = vk::PipelineBindPoint::eGraphics;
        break;
    case Type::Compute:
        bindPoint = vk::PipelineBindPoint::eCompute;
        break;
    }

    return bindPoint;
}

vk::PipelineVertexInputStateCreateInfo Pipeline::updateVertexInput(std::vector<ShaderBinding::InputBinding>& inputs)
{
	vk::PipelineVertexInputStateCreateInfo vertexInputState; 

	// check for empty vertex input
	if (inputs.empty())
	{
		vertexInputState.vertexAttributeDescriptionCount = 0;
		vertexInputState.pVertexAttributeDescriptions = nullptr;
		vertexInputState.vertexBindingDescriptionCount = 0;
		vertexInputState.pVertexBindingDescriptions = nullptr;
		return;
	}
    
    for (const ShaderBinding::InputBinding& input : inputs)
    {
        vertexAttrDescr.push_back({ input.loc, 0, input.format, input.stride });
    }
    
	// first sort the attributes so they are in order of location
	std::sort(vertexAttrDescr.begin(), vertexAttrDescr.end(),
	          [](const vk::VertexInputAttributeDescription lhs, const vk::VertexInputAttributeDescription rhs) {
		          return lhs.location < rhs.location;
	          });

	vertexInputState.vertexAttributeDescriptionCount = vertexAttrDescr.size();
	vertexInputState.pVertexAttributeDescriptions = vertexAttrDescr.data();
	vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindDescr.size());
	vertexInputState.pVertexBindingDescriptions = vertexBindDescr.data();

	return vertexInputState;
}

void Pipeline::addEmptyLayout(VkContext& context)
{
	vk::PipelineLayoutCreateInfo createInfo;
	VK_CHECK_RESULT(context.getDevice().createPipelineLayout(&createInfo, nullptr, &pipelineLayout));
}

void Pipeline::create(VkContext& context, RenderPass& rPass, ShaderProgram& shader)
{ 
	RenderStateBlock* renderState = shader;

	// calculate the offset and stride size
	vk::PipelineVertexInputStateCreateInfo vertInputState = updateVertexInput(shader.inputs);
    
    // ============== primitive topology =====================
    vk::PipelineInputAssemblyStateCreateInfo assemblyState;
	assemblyState.topology = renderState->rastState.topology;
	assemblyState.primitiveRestartEnable = renderState->rastState.primRestart;
    
    // ============== depth/stenicl state ====================
    vk::PipelineDepthStencilStateCreateInfo depthStencilState;
    depthStencilState.depthTestEnable = shader->depth.testEnable;
    depthStencilState.depthWriteEnable = shader->depth.writeEnable;
    depthStencilState.depthCompareOp = shader->depth.compareOp;
    
    // ============== stencil state =====================
    depthStencilState.stencilTestEnable = VK_TRUE;
    depthStencilState.front.failOp = failOp;
    depthStencilState.front.depthFailOp = depthFailOp;
    depthStencilState.front.passOp = passOp;
    depthStencilState.front.compareMask = compareMask;
    depthStencilState.front.writeMask = writeMask;
    depthStencilState.front.reference = ref;
    depthStencilState.front.compareOp = compareOp;
    depthStencilState.back = depthStencilState.front;
    
    // ============ raster state =======================
    vk::PipelineRasterizationStateCreateInfo rasterState;
	rasterState.cullMode = renderState->rastState.cullMode;
	rasterState.frontFace = renderState->rastState.frontFace;
	rasterState.polygonMode = renderState->rastState.polygonMode;
    
    // ============ dynamic states ====================
    vk::PipelineDynamicStateCreateInfo dynamicCreateState;
    dynamicCreateState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicCreateState.pDynamicStates = dynamicStates.data();
    
	// =============== viewport state ====================
	vk::PipelineViewportStateCreateInfo viewportState;
    vk::Viewport viewPort(0.0f, 0.0f, static_cast<float>(renderpass.getImageWidth()), static_cast<float>(renderpass.getImageHeight()), 0.0f,
	                      1.0f);
	vk::Rect2D scissor(vk::Offset2D(0, 0), vk::Extent2D((uint32_t)viewPort.width, (uint32_t)viewPort.height));
	viewportState.pViewports = &viewPort;
	viewportState.viewportCount = 1;
	viewportState.pScissors = &scissor;
	viewportState.scissorCount = 1;
    
    // ============= colour attachment =================
	auto& colAttachments = renderpass.getColourAttachs();
    vk::PipelineColorBlendStateCreateInfo colourBlendState;
	colourBlendState.attachmentCount = static_cast<uint32_t>(colAttachments.size());
	colourBlendState.pAttachments = colAttachments.data();
    
    // aplha blending - using preset states
    if (blendFactor == VK_TRUE)
    {
        // deal with blending here
    }
    
	auto& shaderCreateInfo = shader.getShaderCreateInfo();

    // ================= create the pipeline =======================
	vk::GraphicsPipelineCreateInfo createInfo({}, shaderCreateInfo.size(), shaderCreateInfo.data(), &vertInputState,
	                                          &assemblyState, nullptr, &viewportState, &rasterState, &multiSampleState,
	                                          &depthStencilState, &colorBlendState, &dynamicCreateState, pipelineLayout,
	                                          this->renderpass.get(), 0, nullptr, 0);

	VK_CHECK_RESULT(context.getDevice().createGraphicsPipelines({}, 1, &createInfo, nullptr, &pipeline));
}

}    // namespace VulkanAPI
