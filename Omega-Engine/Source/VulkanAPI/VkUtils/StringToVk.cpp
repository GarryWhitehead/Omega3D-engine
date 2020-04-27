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

#include "StringToVk.h"

#include "utility/Logger.h"

namespace VulkanAPI
{

namespace VkUtils
{

vk::CompareOp vkCompareOpFromString(std::string str)
{
    vk::CompareOp result = vk::CompareOp::eLessOrEqual;
    if (str == "Always")
    {
        result = vk::CompareOp::eAlways;
    }
    else if (str == "Equal")
    {
        result = vk::CompareOp::eEqual;
    }
    else if (str == "Greater")
    {
        result = vk::CompareOp::eGreater;
    }
    else if (str == "GreaterOrEqual")
    {
        result = vk::CompareOp::eGreaterOrEqual;
    }
    else if (str == "Less")
    {
        result = vk::CompareOp::eLess;
    }
    else if (str == "LessOrEqual")
    {
        result = vk::CompareOp::eLessOrEqual;
    }
    else if (str == "Never")
    {
        result = vk::CompareOp::eNever;
    }
    else if (str == "NotEqual")
    {
        result = vk::CompareOp::eNotEqual;
    }
    else
    {
        LOGGER_INFO("Invalid CompareOp command. Setting to LessOrEqual.");
    }
    return result;
}

vk::PolygonMode vkPolygonFromString(std::string str)
{
    vk::PolygonMode result = vk::PolygonMode::eFill;
    if (str == "Fill")
    {
        result = vk::PolygonMode::eFill;
    }
    else if (str == "Line")
    {
        result = vk::PolygonMode::eLine;
    }
    else if (str == "Point")
    {
        result = vk::PolygonMode::ePoint;
    }
    else if (str == "RectangleNV")
    {
        result = vk::PolygonMode::eFillRectangleNV;
    }
    else
    {
        LOGGER_INFO("Invalid polygon mode. Setting to Fill");
    }
    return result;
}

vk::CullModeFlagBits vkCullModeFromString(std::string str)
{
    vk::CullModeFlagBits result = vk::CullModeFlagBits::eNone;
    if (str == "Front")
    {
        result = vk::CullModeFlagBits::eFront;
    }
    else if (str == "Back")
    {
        result = vk::CullModeFlagBits::eBack;
    }
    else if (str == "FrontAndBack")
    {
        result = vk::CullModeFlagBits::eFrontAndBack;
    }
    return result;
}

vk::FrontFace vkFrontFaceFromString(std::string str)
{
    vk::FrontFace result = vk::FrontFace::eCounterClockwise;
    if (str == "Clockwise")
    {
        result = vk::FrontFace::eClockwise;
    }
    else if (str == "CounterClockwise")
    {
        result = vk::FrontFace::eCounterClockwise;
    }
    else
    {
        LOGGER_INFO("Invalid front face value. Setting to CounterClockwise.");
    }
    return result;
}

vk::Filter vkFilterToString(std::string str)
{
    vk::Filter result = vk::Filter::eLinear;
    if (str == "Nearest")
    {
        result = vk::Filter::eNearest;
    }
    else if (str == "Cubic")
    {
        result = vk::Filter::eCubicIMG;
    }
    else if (str != "Linear")
    {
        LOGGER_INFO("Invalid filter value. Setting to Linear");
    }
    return result;
}

vk::SamplerAddressMode vkAddressModeToString(std::string str)
{
    vk::SamplerAddressMode result = vk::SamplerAddressMode::eClampToBorder;
    if (str == "ClampToEdge")
    {
        result = vk::SamplerAddressMode::eClampToEdge;
    }
    else if (str == "Repeat")
    {
        result = vk::SamplerAddressMode::eRepeat;
    }
    else if (str == "MirrorClampToEdge")
    {
        result = vk::SamplerAddressMode::eMirrorClampToEdge;
    }
    else if (str == "MirrorRepeat")
    {
        result = vk::SamplerAddressMode::eMirroredRepeat;
    }
    else if (str != "ClampToBorder")
    {
        LOGGER_INFO("Invalid address mode. Setting to ClampToBorder");
    }
    return result;
}

vk::DescriptorType getVkDescrTypeFromStr(std::string str)
{
    vk::DescriptorType result;
    if (str == "UniformBuffer")
    {
        result = vk::DescriptorType::eUniformBuffer;
    }
    else if (str == "DynamicUniform")
    {
        result = vk::DescriptorType::eUniformBufferDynamic;
    }
    else if (str == "StorageBuffer")
    {
        result = vk::DescriptorType::eStorageBuffer;
    }
    else if (str == "DynamicStorageBuffer")
    {
        result = vk::DescriptorType::eStorageBufferDynamic;
    }
    else if (str == "2D_Sampler" || str == "3D_Sampler" || str == "Cube_Sampler")
    {
        result = vk::DescriptorType::eCombinedImageSampler;
    }
    else
    {
        LOGGER_ERROR("Unsupported buffer type - %s", str.c_str());
        assert(0);
    }
    return result;
}

vk::StencilOp vkStencilOpFromString(std::string str)
{
    vk::StencilOp result = vk::StencilOp::eKeep;
    if (str == "Keep")
    {
        result = vk::StencilOp::eKeep;
    }
    else if (str == "Replace")
    {
        result = vk::StencilOp::eReplace;
    }
    else if (str == "Zero")
    {
        result = vk::StencilOp::eZero;
    }
    else if (str == "Invert")
    {
        result = vk::StencilOp::eInvert;
    }
    else if (str == "DecrementAndWrap")
    {
        result = vk::StencilOp::eDecrementAndWrap;
    }
    else if (str == "IncrementAndWrap")
    {
        result = vk::StencilOp::eIncrementAndWrap;
    }
    else
    {
        LOGGER_WARN("Unrecognsied stencilOp parameter. Setting to Keep.");
    }
    return result;
}

} // namespace VkUtils
} // namespace VulkanAPI
