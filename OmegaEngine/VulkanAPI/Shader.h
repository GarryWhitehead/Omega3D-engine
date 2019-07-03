#pragma once
#include "VulkanAPI/Common.h"
#include "VulkanAPI/Sampler.h"

#include "external/Spirv-Cross/spirv_cross.hpp"

#include <optional>

namespace VulkanAPI
{
// forward decleartions
class DescriptorLayout;
class PipelineLayout;
class Pipeline;
class Sampler;
enum class StageType;

struct BufferReflection
{
	// contains useful information in regards to the bindings, sets and names of buffers, samplers, etc.
	// Avoids having to use the descriptor layouts and offers more flexibility
	struct ShaderBufferLayout
	{
		vk::DescriptorType type;
		uint32_t binding;
		uint32_t set;
		size_t range;
		std::string name;

		ShaderBufferLayout(vk::DescriptorType _type, uint32_t _bind, uint32_t _set,
		                   std::string _name, size_t _range)
		    : type(_type)
		    , binding(_bind)
		    , set(_set)
		    , name(_name)
		    , range(_range)
		{
		}
	};

	std::vector<ShaderBufferLayout> layouts;

	std::optional<ShaderBufferLayout> find(uint32_t set, uint32_t binding)
	{
		for (auto &layout : layouts)
		{
			if (layout.set == set && layout.binding == binding)
			{
				return layout;
			}
		}
		return std::nullopt;
	}
};

struct ImageReflection
{
	struct ShaderImageLayout
	{
		uint32_t binding;
		uint32_t set;
		std::string name;
		vk::ImageLayout layout;
		vk::DescriptorType type;
		Sampler sampler;

		ShaderImageLayout()
		{
		}
		ShaderImageLayout(vk::DescriptorType _type, vk::ImageLayout lo, uint32_t bind, uint32_t s,
		                  std::string n, Sampler spl)
		    : type(_type)
		    , layout(lo)
		    , binding(bind)
		    , set(s)
		    , name(n)
		    , sampler(spl)
		{
		}
	};

	std::vector<ShaderImageLayout> layouts;

	std::optional<ShaderImageLayout> find(uint32_t set, uint32_t binding)
	{
		for (auto &layout : layouts)
		{
			if (layout.set == set && layout.binding == binding)
			{
				return layout;
			}
		}
		return std::nullopt;
	}
};

enum class StageType
{
	Vertex,
	Fragment,
	Geometry,
	Compute,
	Count
};

class Shader
{

public:
	Shader();
	~Shader();

	uint32_t size() const
	{
		return static_cast<uint32_t>(wrappers.size());
	}

	vk::PipelineShaderStageCreateInfo *getPipelineData()
	{
		return (vk::PipelineShaderStageCreateInfo *)wrappers.data();
	}

	Sampler getSamplerType(std::string name);
	vk::ImageLayout getImageLayout(std::string name);
	std::tuple<vk::Format, uint32_t> getTypeFormat(uint32_t width, uint32_t vecSize,
	                                               spirv_cross::SPIRType::BaseType type);

	static vk::ShaderStageFlagBits getStageFlags(StageType type);

	bool add(vk::Device device, const std::string &filename, StageType type);
	bool add(vk::Device device, const std::string &filename1, StageType type1,
	         const std::string &filename2, StageType type2);
	void bufferReflection(DescriptorLayout &descriptorLayout, BufferReflection &bufferReflection);
	void imageReflection(DescriptorLayout &descriptorLayout, ImageReflection &imageReflect);
	void pipelineLayoutReflect(PipelineLayout &layout);
	void pipelineReflection(Pipeline &pipeline);

	std::vector<uint32_t> getData(StageType type)
	{
		return data[(int)type];
	}

private:
	bool loadShaderBinary(const char *filename, StageType type);
	void createModule(vk::Device device, StageType type);
	void createWrapper(StageType type);

	vk::Device device;
	std::vector<vk::PipelineShaderStageCreateInfo> wrappers;

	std::array<vk::ShaderModule, (int)StageType::Count> modules;
	std::array<std::vector<uint32_t>, (int)StageType::Count> data;
	std::array<bool, (int)StageType::Count> currentStages = { false };
};

} // namespace VulkanAPI
