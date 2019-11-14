#include "Pipeline.h"

#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/RenderPass.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/VkDriver.h"

#include "spirv_cross.hpp"

namespace VulkanAPI
{

PipelineLayout::PipelineLayout()
{
}

void PipelineLayout::prepare(VkDriver& device,
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
        Shader::StageType type = pConstantSizes[i].second;
        
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

void Pipeline::updateVertexInput(std::vector<ShaderProgram::InputBinding>& inputs)
{
	// check for empty vertex input
	if (inputs.empty())
	{
		vertexInputState.vertexAttributeDescriptionCount = 0;
		vertexInputState.pVertexAttributeDescriptions = nullptr;
		vertexInputState.vertexBindingDescriptionCount = 0;
		vertexInputState.pVertexBindingDescriptions = nullptr;
		return;
	}
    
    for (const ShaderProgram::InputBinding& input : inputs)
    {
        vertexAttrDescr.push_back({ input.loc, 0, input.format, input.stride });
    }
    
	// first sort the attributes so they are in order of location
	std::sort(vertexAttrDescr.begin(), vertexAttrDescr.end(),
	          [](const vk::VertexInputAttributeDescription lhs, const vk::VertexInputAttributeDescription rhs) {
		          return lhs.loc < rhs.loc;
	          });

	// calculate the offset for each location - the size of each location is stored temporarily in the offset elemnt of the struct
	uint32_t nextOffset = 0;
	uint32_t currentOffset = 0;
	uint32_t totalSize = 0;
	uint32_t attributeCount = static_cast<uint32_t>(vertexAttrDescr.size());

	for (uint32_t i = 0; i < attributeCount; ++i)
	{
		nextOffset = vertexAttrDescr[i].offset;
		vertexAttrDescr[i].offset = currentOffset;
		currentOffset += nextOffset;
		totalSize += nextOffset;
	}

	// assuming just one binding at the moment
	vk::VertexInputBindingDescription bind_descr(0, totalSize,
	                                             vk::VertexInputRate::eVertex);    // should also support instancing
	vertexBindDescr.push_back(bind_descr);

	vertexInputState.vertexAttributeDescriptionCount = attributeCount;
	vertexInputState.pVertexAttributeDescriptions = vertexAttrDescr.data();
	vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindDescr.size());
	vertexInputState.pVertexBindingDescriptions = vertexBindDescr.data();
}

void Pipeline::addEmptyLayout()
{
	vk::PipelineLayoutCreateInfo createInfo;
	VK_CHECK_RESULT(device.createPipelineLayout(&createInfo, nullptr, &pipelineLayout));
}

void Pipeline::create(VkDriver& dev, RenderPass& rPass, ShaderProgram& shader)
{
	
	type = type;
	this->renderpass = renderpass;
	this->pipelineLayout = layout.get();
    
    vk::device = dev;
    
	// calculate the offset and stride size
	updateVertexInput(shader->inputs);
    
    // ============== primitive topology =====================
    vk::PipelineInputAssemblyStateCreateInfo assemblyState;
    assemblyState.topology = shader->topology;
    assemblyState.primitiveRestartEnable = shader->primRestart;
    
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
    rasterState.cullMode = shader->rasterState.cullMode;
    rasterState.frontFace = shader->rasterState.frontFace;
    rasterState.polygonMode = shader->rasterState.polygonMode;
    
    // ============ dynamic states ====================
    vk::PipelineDynamicStateCreateInfo dynamicCreateState;
    dynamicCreateState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicCreateState.pDynamicStates = dynamicStates.data();
    
	// =============== viewport state ====================
	vk::PipelineViewportStateCreateInfo viewportState;
    vk::Viewport viewPort(0.0f, 0.0f, (float)renderpass.getImageWidth(), (float)renderpass.getImageHeight(), 0.0f,
	                      1.0f);
	vk::Rect2D scissor(vk::Offset2D(0, 0), vk::Extent2D((uint32_t)viewPort.width, (uint32_t)viewPort.height));
	viewportState.pViewports = &viewPort;
	viewportState.viewportCount = 1;
	viewportState.pScissors = &scissor;
	viewportState.scissorCount = 1;
    
    // ============= colour attachment =================
    vk::PipelineColorBlendStateCreateInfo colorBlendState;
    
    // for each clear colour state in the renderpass, we need a blend attachment
    for (uint32_t i = 0; i < renderpass.get_attach_count(); ++i)
    {
        vk::PipelineColorBlendAttachmentState colour;
        colour.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        colour.blendEnable = blendFactor;
        colorAttachState.push_back(colour);
    }
    
    // aplha blending - using preset states
    if (blendFactor == VK_TRUE)
    {
        // deal with blending here
    }

    // update blend state
    colorBlendState.attachmentCount = static_cast<uint32_t>(colorAttachState.size());
    colorBlendState.pAttachments = colorAttachState.data();
    
    // ================= create the pipeline =======================
	vk::GraphicsPipelineCreateInfo createInfo({}, shader->size(), shader.getShaderData(), &vertexInputState,
	                                          &assemblyState, nullptr, &viewportState, &rasterState, &multiSampleState,
	                                          &depthStencilState, &colorBlendState, &dynamicCreateState, pipelineLayout,
	                                          this->renderpass.get(), 0, nullptr, 0);

	VK_CHECK_RESULT(device.createGraphicsPipelines({}, 1, &createInfo, nullptr, &pipeline));
}

}    // namespace VulkanAPI
