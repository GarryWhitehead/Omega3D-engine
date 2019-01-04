#include "Shader.h"
#include "utility/logger.h"

#include <fstream>

namespace VulkanAPI
{

	Shader::Shader(vk::Device device, const char* filename, vk::ShaderStageFlagBits stage)
	{
		loadShaderBinary(filename);
		createModule(device);
		createWrapper(stage);
	}


	Shader::~Shader()
	{
	}

	void Shader::loadShaderBinary(const char* filename)
	{
		std::string shaderDir("assets/shaders/");
		std::ifstream file(shaderDir + filename, std::ios_base::ate | std::ios_base::binary);
		if (!file.is_open()) {
			LOGGER_ERROR("Unable to open shder %s.", filename);
			throw std::runtime_error("Unable to init shader.");
		}

		std::ifstream::pos_type filePos = file.tellg();
		data.resize(filePos);
		file.seekg(0, std::ios_base::beg);
		file.read((char*)data.data(), filePos);
	}

	void Shader::createModule(vk::Device device)
	{
		vk::ShaderModuleCreateInfo shaderInfo({}, static_cast<uint32_t>(data.size()), data.data());
		
		VK_CHECK_RESULT(device.createShaderModule(&shaderInfo, nullptr, &module));

	}

	void Shader::createWrapper(vk::ShaderStageFlagBits stage)
	{
		vk::PipelineShaderStageCreateInfo createInfo({}, stage, module, "main", nullptr);
		wrapper = createInfo;
	}
}
