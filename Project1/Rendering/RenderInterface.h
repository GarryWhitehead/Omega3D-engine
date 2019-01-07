#pragma once

#include "Rendering/RenderableTypes.h"
#include "Vulkan/Device.h"
#include "Vulkan/Interface.h"
#include "Vulkan/Shader.h"
#include "Vulkan/Pipeline.h"
#include <vector>
#include <memory>

namespace OmegaEngine
{	
	// forward decleartions
	class ComponentInterface;
	class Object;
	enum class RenderableType;

	struct PipelineInfo
	{
		VulkanAPI::Shader shader;
		VulkanAPI::PipelineLayout pl_layout;
		VulkanAPI::Pipeline pipeline;
		VulkanAPI::DescriptorLayout descr_layout;
	};

	class RenderInterface
	{

	public:

		RenderInterface(VulkanAPI::Device device, const uint32_t win_width, const uint32_t win_height);
		~RenderInterface();

		// renderable type creation
		void add_static_mesh(std::unique_ptr<ComponentInterface>& comp_interface, Object& obj);

		// shader init for eacg renderable type
		void add_shader(RenderTypes type);

	private:

		std::vector<RenderableType> renderables;

		std::unique_ptr<VulkanAPI::Interface> vk_interface;

		// all the pipelines and shaders for each renderable type
		std::array<PipelineInfo, (int)OmegaEngine::RenderTypes::Count> pipelines;
	};

}

