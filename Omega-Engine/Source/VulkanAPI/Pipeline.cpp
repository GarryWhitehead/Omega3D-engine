#include "Pipeline.h"

#include "VulkanAPI/RenderPass.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/VkContext.h"
#include "VulkanAPI/VkDriver.h"
#include "utility/Logger.h"

namespace VulkanAPI
{

PipelineLayout::PipelineLayout()
{
}

void PipelineLayout::prepare(VkContext& context)
{
    // create push constants - just the size for now. The data contents are set at draw time
    std::vector<vk::PushConstantRange> pConstants;
    vk::ShaderStageFlags flags;
    uint32_t totalSize = 0;

    for (size_t i = 0; i < pConstantSizes.size(); ++i)
    {
        size_t size = pConstantSizes[i].second;
        assert(size > 0);
        Shader::Type type = pConstantSizes[i].first;

        flags |= Shader::getStageFlags(type);
        totalSize += size;
    }

    if (totalSize > 0)
    {
        vk::PushConstantRange push(flags, 0, totalSize);
        pConstants.push_back(push);
    }

    vk::PipelineLayoutCreateInfo pipelineInfo(
        {},
        static_cast<uint32_t>(descriptorLayouts.size()),
        descriptorLayouts.data(),
        static_cast<uint32_t>(pConstants.size()),
        pConstants.data());

    VK_CHECK_RESULT(context.device.createPipelineLayout(&pipelineInfo, nullptr, &layout));
}

void PipelineLayout::addPushConstant(Shader::Type stage, uint32_t size)
{
    pConstantSizes.emplace_back(std::make_pair(stage, size));
}

void PipelineLayout::addDescriptorLayout(const vk::DescriptorSetLayout& layout)
{
    descriptorLayouts.emplace_back(layout);
}

vk::PipelineLayout& PipelineLayout::get()
{
    return layout;
}

// ================== pipeline =======================
Pipeline::Pipeline(
    VkContext& context, RenderPass& rpass, PipelineLayout& layout, Pipeline::Type type)
    : context(context), renderpass(rpass), pipelineLayout(layout), type(type)
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
        default:
            LOGGER_ERROR("Unrecognised pipeline type; type id: %i", static_cast<int>(type));
    }

    return bindPoint;
}

vk::PipelineVertexInputStateCreateInfo
Pipeline::updateVertexInput(std::vector<ShaderProgram::InputBinding>& inputs)
{
    vk::PipelineVertexInputStateCreateInfo vertexInputState;

    // check for empty vertex input
    if (inputs.empty())
    {
        vertexInputState.vertexAttributeDescriptionCount = 0;
        vertexInputState.pVertexAttributeDescriptions = nullptr;
        vertexInputState.vertexBindingDescriptionCount = 0;
        vertexInputState.pVertexBindingDescriptions = nullptr;
        return vertexInputState;
    }

    uint32_t offset = 0;
    for (const ShaderProgram::InputBinding& input : inputs)
    {
        vertexAttrDescr.push_back({input.loc, 0, input.format, offset});
        offset += input.stride;

        vertexBindDescr.push_back({input.loc, input.stride, vk::VertexInputRate::eVertex});
    }

    // first sort the attributes so they are in order of location
    std::sort(
        vertexAttrDescr.begin(),
        vertexAttrDescr.end(),
        [](const vk::VertexInputAttributeDescription lhs,
           const vk::VertexInputAttributeDescription rhs) { return lhs.location < rhs.location; });

    vertexInputState.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(vertexAttrDescr.size());
    vertexInputState.pVertexAttributeDescriptions = vertexAttrDescr.data();
    vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindDescr.size());
    vertexInputState.pVertexBindingDescriptions = vertexBindDescr.data();

    return vertexInputState;
}

void Pipeline::addEmptyLayout()
{
    vk::PipelineLayoutCreateInfo createInfo;
    VK_CHECK_RESULT(
        context.device.createPipelineLayout(&createInfo, nullptr, &pipelineLayout.get()));
}

void Pipeline::create(ShaderProgram& program)
{
    auto& renderState = program.renderState;

    // prepare the pipeline layout
    pipelineLayout.prepare(context);

    // calculate the offset and stride size
    vk::PipelineVertexInputStateCreateInfo vertInputState = updateVertexInput(program.inputs);

    // ============== primitive topology =====================
    vk::PipelineInputAssemblyStateCreateInfo assemblyState;
    assemblyState.topology = renderState->rastState.topology;
    assemblyState.primitiveRestartEnable = renderState->rastState.primRestart;

    // ============== multi-sample state =====================
    vk::PipelineMultisampleStateCreateInfo sampleState;

    // ============== depth/stenicl state ====================
    vk::PipelineDepthStencilStateCreateInfo depthStencilState;
    depthStencilState.depthTestEnable = renderState->dsState.testEnable;
    depthStencilState.depthWriteEnable = renderState->dsState.writeEnable;
    depthStencilState.depthCompareOp = renderState->dsState.compareOp;

    // ============== stencil state =====================
    depthStencilState.stencilTestEnable = renderState->dsState.stencilTestEnable;
    if (renderState->dsState.stencilTestEnable)
    {
        depthStencilState.front.failOp = renderState->dsState.front.failOp;
        depthStencilState.front.depthFailOp = renderState->dsState.front.depthFailOp;
        depthStencilState.front.passOp = renderState->dsState.front.passOp;
        depthStencilState.front.compareMask = renderState->dsState.front.compareMask;
        depthStencilState.front.writeMask = renderState->dsState.front.writeMask;
        depthStencilState.front.reference = renderState->dsState.front.reference;
        depthStencilState.front.compareOp = renderState->dsState.front.compareOp;
        depthStencilState.back = depthStencilState.front;
    }


    // ============ raster state =======================
    vk::PipelineRasterizationStateCreateInfo rasterState;
    rasterState.cullMode = renderState->rastState.cullMode;
    rasterState.frontFace = renderState->rastState.frontFace;
    rasterState.polygonMode = renderState->rastState.polygonMode;
    rasterState.lineWidth = 1.0f;

    // ============ dynamic states ====================
    vk::PipelineDynamicStateCreateInfo dynamicCreateState;
    dynamicCreateState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicCreateState.pDynamicStates = dynamicStates.data();

    // =============== viewport state ====================
    vk::PipelineViewportStateCreateInfo viewportState;
    vk::Viewport viewPort(
        0.0f,
        0.0f,
        static_cast<float>(renderpass.getWidth()),
        static_cast<float>(renderpass.getHeight()),
        0.0f,
        1.0f);
    vk::Rect2D scissor(
        vk::Offset2D(0, 0), vk::Extent2D((uint32_t) viewPort.width, (uint32_t) viewPort.height));
    viewportState.pViewports = &viewPort;
    viewportState.viewportCount = 1;
    viewportState.pScissors = &scissor;
    viewportState.scissorCount = 1;

    // ============= colour attachment =================
    auto colAttachments = renderpass.getColourAttachs();
    vk::PipelineColorBlendStateCreateInfo colourBlendState;
    colourBlendState.attachmentCount = static_cast<uint32_t>(colAttachments.size());
    colourBlendState.pAttachments = colAttachments.data();

    // aplha blending - using preset states
    // if (blendFactor == VK_TRUE)
    //{
    // deal with blending here
    // }

    std::vector<vk::PipelineShaderStageCreateInfo> shaderData;
    for (auto& stage : program.stages)
    {
        shaderData.emplace_back(stage.getShader()->get());
    }

    // ================= create the pipeline =======================
    assert(pipelineLayout.get());

    vk::GraphicsPipelineCreateInfo createInfo(
        {},
        static_cast<uint32_t>(shaderData.size()),
        shaderData.data(),
        &vertInputState,
        &assemblyState,
        nullptr,
        &viewportState,
        &rasterState,
        &sampleState,
        &depthStencilState,
        &colourBlendState,
        &dynamicCreateState,
        pipelineLayout.get(),
        renderpass.get(),
        0,
        nullptr,
        0);

    VK_CHECK_RESULT(context.device.createGraphicsPipelines({}, 1, &createInfo, nullptr, &pipeline));
}

} // namespace VulkanAPI
