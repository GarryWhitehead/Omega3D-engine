#include "Shader.h"
#include "utility/logger.h"

#include <fstream>

namespace VulkanAPI
{
	Shader::Shader()
	{

	}

	Shader::~Shader()
	{
	}

	static vk::ShaderStageFlagBits get_stage_flag_bits(StageType type)
	{
		vk::ShaderStageFlagBits ret;
		switch (type) {
		case StageType::Vertex:
			ret = vk::ShaderStageFlagBits::eVertex;
			break;
		case StageType::Fragment:
			ret = vk::ShaderStageFlagBits::eFragment;
			break;
		case StageType::Geometry:
			ret = vk::ShaderStageFlagBits::eGeometry;
			break;
		case StageType::Compute:
			ret = vk::ShaderStageFlagBits::eCompute;
			break;
		}
		return ret;
	}

	bool Shader::add(vk::Device device, const char* filename, StageType type)
	{
		if (!loadShaderBinary(filename, type)) {
			return false;
		}

		createModule(device, type);
		createWrapper(type);
		return true;
	}

	bool Shader::add(vk::Device device, const char* filename1, StageType type1, const char* filename2, StageType type2)
	{
		if (!add(device, filename1, type1)) {
			return false;
		}
		if (!add(device, filename2, type2)) {
			return false;
		}
		return true;
	}

	bool Shader::loadShaderBinary(const char* filename, StageType type)
	{
		std::string shaderDir("assets/shaders/");
		std::ifstream file(shaderDir + filename, std::ios_base::ate | std::ios_base::binary);
		if (!file.is_open()) {
			return false;
		}

		std::ifstream::pos_type filePos = file.tellg();
		data[(int)type].resize(filePos);
		file.seekg(0, std::ios_base::beg);
		file.read((char*)data.data(), filePos);
		return true;
	}

	void Shader::createModule(vk::Device device, StageType type)
	{
		vk::ShaderModuleCreateInfo shaderInfo({}, static_cast<uint32_t>(data[(int)type].size()), data[(int)type].data());
		
		VK_CHECK_RESULT(device.createShaderModule(&shaderInfo, nullptr, &modules[(int)type]));

	}

	void Shader::createWrapper(StageType type)
	{
		vk::ShaderStageFlagBits stage = get_stage_flag_bits(type);
		vk::PipelineShaderStageCreateInfo createInfo({}, stage, modules[(int)type], "main", nullptr);
		wrappers[(int)type] = createInfo;
	}
}
