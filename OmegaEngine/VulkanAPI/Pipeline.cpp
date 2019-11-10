#include "Pipeline.h"

#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/RenderPass.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/VkContext.h"

#include "spirv_cross.hpp"

namespace VulkanAPI
{

PipelineLayout::PipelineLayout()
{
}

void PipelineLayout::prepare(VkContext& device,
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
	// setup defualt pipeline states
	addDynamicState(vk::DynamicState::eScissor);
	addDynamicState(vk::DynamicState::eViewport);

	rasterState.lineWidth = 1.0f;
}

Pipeline::~Pipeline()
{
}

void Pipeline::addVertexInput(uint32_t location, vk::Format format, uint32_t size)
{
	// offsets will be calculated just before pipeline creation
	vk::VertexInputAttributeDescription attr_descr(location, 0, format, size);
	vertexAttrDescr.push_back(attr_descr);
}

void Pipeline::updateVertexInput()
{
	// check for empty vertex
	if (vertexAttrDescr.empty())
	{
		vertexInputState.vertexAttributeDescriptionCount = 0;
		vertexInputState.pVertexAttributeDescriptions = nullptr;
		vertexInputState.vertexBindingDescriptionCount = 0;
		vertexInputState.pVertexBindingDescriptions = nullptr;
		return;
	}

	// first sort the attributes so they are in order of location as when we reflect, we can get the inputs in any order
	std::sort(vertexAttrDescr.begin(), vertexAttrDescr.end(),
	          [](const vk::VertexInputAttributeDescription lhs, const vk::VertexInputAttributeDescription rhs) {
		          return lhs.location < rhs.location;
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

void Pipeline::setRasterCullMode(vk::CullModeFlags cullMode)
{
	rasterState.cullMode = cullMode;
}

void Pipeline::setRasterFrontFace(vk::FrontFace frontFace)
{
	rasterState.frontFace = frontFace;
}

void Pipeline::setRasterDepthClamp(bool state)
{
	rasterState.depthBiasClamp = state;
}

void Pipeline::setTopology(const OmegaEngine::StateTopology& topology)
{
	switch (topology)
	{
	case OmegaEngine::StateTopology::List:
		assemblyState.topology = vk::PrimitiveTopology::eTriangleList;
		break;
	case OmegaEngine::StateTopology::Strip:
		assemblyState.topology = vk::PrimitiveTopology::eTriangleStrip;
		break;
	case OmegaEngine::StateTopology::StripRestart:
		assemblyState.topology = vk::PrimitiveTopology::eTriangleStrip;
		assemblyState.primitiveRestartEnable = VK_TRUE;
		break;
	}
}

void Pipeline::addColourAttachment(bool blendFactor, RenderPass& renderpass)
{
	for (uint32_t i = 0; i < renderpass.get_attach_count(); ++i)
	{
		vk::PipelineColorBlendAttachmentState colour;
		colour.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
		                        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		colour.blendEnable = blendFactor;
		colorAttachState.push_back(colour);
	}

	if (blendFactor == VK_TRUE)
	{
		// deal with blending here
	}

	// update blend state
	colorBlendState.attachmentCount = static_cast<uint32_t>(colorAttachState.size());
	colorBlendState.pAttachments = colorAttachState.data();
}

void Pipeline::addDynamicState(vk::DynamicState state)
{
	// this needs to be enabled if using depth bias - TODO: should also check this is supported by the hardware
	if (state == vk::DynamicState::eDepthBias)
	{
		rasterState.depthBiasEnable = VK_TRUE;
	}

	dynamicStates.push_back(state);
	dynamicCreateState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicCreateState.pDynamicStates = dynamicStates.data();
}

void Pipeline::setDepthState(bool testState, bool writeState, vk::CompareOp compare)
{
	depthStencilState.depthTestEnable = testState;
	depthStencilState.depthWriteEnable = writeState;
	depthStencilState.depthCompareOp = compare;
}

void Pipeline::setStencilStateFrontAndBack(vk::CompareOp compareOp, vk::StencilOp failOp, vk::StencilOp depthFailOp,
                                           vk::StencilOp passOp, uint32_t compareMask, uint32_t writeMask, uint32_t ref)
{
	depthStencilState.stencilTestEnable = VK_TRUE;
	depthStencilState.front.failOp = failOp;
	depthStencilState.front.depthFailOp = depthFailOp;
	depthStencilState.front.passOp = passOp;
	depthStencilState.front.compareMask = compareMask;
	depthStencilState.front.writeMask = writeMask;
	depthStencilState.front.reference = ref;
	depthStencilState.front.compareOp = compareOp;
	depthStencilState.back = depthStencilState.front;
}

void Pipeline::setRenderpass(RenderPass& pass)
{
	renderpass = pass;
}

void Pipeline::addShader(Shader& shader)
{
	this->shader = shader;
}

void Pipeline::addLayout(vk::PipelineLayout& layout)
{
	assert(layout);
	pipelineLayout = layout;
}

void Pipeline::addEmptyLayout()
{
	vk::PipelineLayoutCreateInfo createInfo;
	VK_CHECK_RESULT(device.createPipelineLayout(&createInfo, nullptr, &pipelineLayout));
}

void Pipeline::create(vk::Device dev, RenderPass& renderpass, Shader& shader, PipelineLayout& layout,
                      PipelineType _type)
{
	device = dev;
	type = _type;
	this->renderpass = renderpass;
	this->pipelineLayout = layout.get();

	// calculate the offset and stride size
	updateVertexInput();

	// use the image size form the renderpass to construct the viewport. Will probably want to offer more methods in the future?
	vk::Viewport viewPort(0.0f, 0.0f, (float)renderpass.getImageWidth(), (float)renderpass.getImageHeight(), 0.0f,
	                      1.0f);
	vk::Rect2D scissor(vk::Offset2D(0, 0), vk::Extent2D((uint32_t)viewPort.width, (uint32_t)viewPort.height));
	viewportState.pViewports = &viewPort;
	viewportState.viewportCount = 1;
	viewportState.pScissors = &scissor;
	viewportState.scissorCount = 1;

	vk::GraphicsPipelineCreateInfo createInfo({}, shader.size(), shader.getPipelineData(), &vertexInputState,
	                                          &assemblyState, nullptr, &viewportState, &rasterState, &multiSampleState,
	                                          &depthStencilState, &colorBlendState, &dynamicCreateState, pipelineLayout,
	                                          this->renderpass.get(), 0, nullptr, 0);

	VK_CHECK_RESULT(device.createGraphicsPipelines({}, 1, &createInfo, nullptr, &pipeline));
}

void Pipeline::create(vk::Device dev, RenderPass& renderpass, Shader& shader, PipelineType _type)
{
	device = dev;
	type = _type;
	this->renderpass = renderpass;

	// no pipeline layout specified so create empty default layout
	vk::PipelineLayoutCreateInfo layoutCreateInfo;
	VK_CHECK_RESULT(device.createPipelineLayout(&layoutCreateInfo, nullptr, &pipelineLayout));

	// calculate the offset and stride size
	updateVertexInput();

	// use the image size form the renderpass to construct the viewport. Will probably want to offer more methods in the future?
	vk::Viewport viewPort(0.0f, 0.0f, (float)renderpass.getImageWidth(), (float)renderpass.getImageHeight(), 0.0f,
	                      1.0f);
	vk::Rect2D scissor(vk::Offset2D(0, 0), vk::Extent2D((uint32_t)viewPort.width, (uint32_t)viewPort.height));
	viewportState.pViewports = &viewPort;
	viewportState.viewportCount = 1;
	viewportState.pScissors = &scissor;
	viewportState.scissorCount = 1;

	vk::GraphicsPipelineCreateInfo createInfo({}, shader.size(), shader.getPipelineData(), &vertexInputState,
	                                          &assemblyState, nullptr, &viewportState, &rasterState, &multiSampleState,
	                                          &depthStencilState, &colorBlendState, &dynamicCreateState,
	                                          pipelineLayout,    // default empty pipeline layout used
	                                          this->renderpass.get(), 0, nullptr, 0);

	VK_CHECK_RESULT(device.createGraphicsPipelines({}, 1, &createInfo, nullptr, &pipeline));
}

void Pipeline::create(vk::Device dev, PipelineType _type)
{
	device = dev;
	type = _type;

	// calculate the offset and stride size
	updateVertexInput();

	// use the image size form the renderpass to construct the viewport. Will probably want to offer more methods in the future?
	vk::Viewport viewPort(0.0f, 0.0f, (float)renderpass.getImageWidth(), (float)renderpass.getImageHeight(), 0.0f,
	                      1.0f);
	vk::Rect2D scissor(vk::Offset2D(0, 0), vk::Extent2D((uint32_t)viewPort.width, (uint32_t)viewPort.height));
	viewportState.pViewports = &viewPort;
	viewportState.viewportCount = 1;
	viewportState.pScissors = &scissor;
	viewportState.scissorCount = 1;

	vk::GraphicsPipelineCreateInfo createInfo({}, shader.size(), shader.getPipelineData(), &vertexInputState,
	                                          &assemblyState, nullptr, &viewportState, &rasterState, &multiSampleState,
	                                          &depthStencilState, &colorBlendState, &dynamicCreateState, pipelineLayout,
	                                          this->renderpass.get(), 0, nullptr, 0);

	VK_CHECK_RESULT(device.createGraphicsPipelines({}, 1, &createInfo, nullptr, &pipeline));
}

// ========= static functions ==============
void Pipeline::reflect(uint32_t* data, size_t dataSize, Pipeline& pipeline)
{
	spirv_cross::Compiler compiler(data, dataSize / sizeof(uint32_t));

	auto shaderResources = compiler.get_shader_resources();

	// get the number of input and output stages
	for (auto& input : shaderResources.stage_inputs)
	{
		uint32_t location = compiler.get_decoration(input.id, spv::DecorationLocation);
		auto& base_type = compiler.get_type(input.base_type_id);
		auto& member = compiler.get_type(base_type.self);

		if (member.vecsize && member.columns == 1)
		{
			std::tuple<vk::Format, uint32_t> info = getTypeFormat(member.width, member.vecsize, member.basetype);
			pipeline.addVertexInput(location, std::get<0>(info), std::get<1>(info));
		}
	}
	/*for (auto& output : shaderResources.stage_outputs) {

			uint32_t location = compiler.get_decoration(output.id, spv::DecorationLocation);
		}*/
}

}    // namespace VulkanAPI
