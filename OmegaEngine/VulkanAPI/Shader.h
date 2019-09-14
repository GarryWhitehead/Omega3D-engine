#pragma once
#include "VulkanAPI/Common.h"
#include "VulkanAPI/Sampler.h"



#include <optional>

namespace VulkanAPI
{
// forward decleartions
class DescriptorLayout;
class PipelineLayout;
class Pipeline;
class Sampler;
enum class StageType;

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
	vk::ImageLayout getImageLayout(std::string& name);
	std::tuple<vk::Format, uint32_t> getTypeFormat(uint32_t width, uint32_t vecSize,
	                                               spirv_cross::SPIRType::BaseType type);

	static vk::ShaderStageFlagBits getStageFlags(StageType type);

	bool add(vk::Device device, const std::string &filename, StageType type);
	bool add(vk::Device device, const std::string &filename1, StageType type1,
	         const std::string &filename2, StageType type2);

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
