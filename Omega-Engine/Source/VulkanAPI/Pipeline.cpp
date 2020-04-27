/* Copyright (c) 2018-2020 Garry Whitehead
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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

    std::sort(descriptorLayouts.begin(), descriptorLayouts.end(), [](std::pair<uint8_t, vk::DescriptorSetLayout>& lhs, std::pair<uint8_t, vk::DescriptorSetLayout>& rhs) { return lhs < rhs; });
    
    std::vector<vk::DescriptorSetLayout> layouts(descriptorLayouts.size());
    for (size_t i = 0; i < descriptorLayouts.size(); ++i)
    {
        layouts[i] = descriptorLayouts[i].second;
    }
    
    vk::PipelineLayoutCreateInfo pipelineInfo(
        {},
        static_cast<uint32_t>(layouts.size()),
        layouts.data(),
        static_cast<uint32_t>(pConstants.size()),
        pConstants.data());

    VK_CHECK_RESULT(context.device.createPipelineLayout(&pipelineInfo, nullptr, &layout));
}

void PipelineLayout::addPushConstant(Shader::Type stage, uint32_t size)
{
    pConstantSizes.emplace_back(std::make_pair(stage, size));
}

void PipelineLayout::addDescriptorLayout(uint8_t set, const vk::DescriptorSetLayout& layout)
{
    descriptorLayouts.push_back({set, layout});
}

vk::PipelineLayout& PipelineLayout::get()
{
    return layout;
}

// ================== pipeline =======================
Pipeline::Pipeline(
    VkContext& context, PipelineLayout& layout, Pipeline::Type type)
    : context(context), pipelineLayout(layout), type(type)
{
}

Pipeline::~Pipeline()
{
    context.device.destroy(pipeline, nullptr);
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
    uint32_t stride = 0;
    for (const ShaderProgram::InputBinding& input : inputs)
    {
        vertexAttrDescr.push_back({input.loc, 0, input.format, offset});
        offset = input.stride;
        stride += input.stride;
    }
    
    // only one binding supported (at binding point 0)
    vertexBindDescr = vk::VertexInputBindingDescription {0, stride, vk::VertexInputRate::eVertex};

    // first sort the attributes so they are in order of location
    std::sort(
        vertexAttrDescr.begin(),
        vertexAttrDescr.end(),
        [](const vk::VertexInputAttributeDescription lhs,
           const vk::VertexInputAttributeDescription rhs) { return lhs.location < rhs.location; });

    vertexInputState.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(vertexAttrDescr.size());
    vertexInputState.pVertexAttributeDescriptions = vertexAttrDescr.data();
    vertexInputState.vertexBindingDescriptionCount = 1;
    vertexInputState.pVertexBindingDescriptions = &vertexBindDescr;

    return vertexInputState;
}

void Pipeline::addEmptyLayout()
{
    vk::PipelineLayoutCreateInfo createInfo;
    VK_CHECK_RESULT(
        context.device.createPipelineLayout(&createInfo, nullptr, &pipelineLayout.get()));
}

void Pipeline::create(ShaderProgram& program, RenderPass* renderpass, FrameBuffer* fbo)
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
        depthStencilState.front.failOp = renderState->dsState.frontStencil.failOp;
        depthStencilState.front.depthFailOp = renderState->dsState.frontStencil.depthFailOp;
        depthStencilState.front.passOp = renderState->dsState.frontStencil.passOp;
        depthStencilState.front.compareMask = renderState->dsState.frontStencil.compareMask;
        depthStencilState.front.writeMask = renderState->dsState.frontStencil.writeMask;
        depthStencilState.front.reference = renderState->dsState.frontStencil.reference;
        depthStencilState.front.compareOp = renderState->dsState.frontStencil.compareOp;
        // TODO: allow the back stencil to differ from the front as this is the only option at present
        if (renderState->dsState.frontStencil.frontEqualBack)
        {
            depthStencilState.back = depthStencilState.front;
        }
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
        static_cast<float>(fbo->getWidth()),
        static_cast<float>(fbo->getHeight()),
        0.0f,
        1.0f);
    vk::Rect2D scissor(
        vk::Offset2D(0, 0), vk::Extent2D((uint32_t) viewPort.width, (uint32_t) viewPort.height));
    viewportState.pViewports = &viewPort;
    viewportState.viewportCount = 1;
    viewportState.pScissors = &scissor;
    viewportState.scissorCount = 1;

    // ============= colour attachment =================
    auto colAttachments = renderpass->getColourAttachs();
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
        renderpass->get(),
        0,
        nullptr,
        0);

    VK_CHECK_RESULT(context.device.createGraphicsPipelines({}, 1, &createInfo, nullptr, &pipeline));
}

} // namespace VulkanAPI
