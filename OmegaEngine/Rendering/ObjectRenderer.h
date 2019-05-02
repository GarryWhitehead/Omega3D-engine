#pragma once

namespace VulkanAPI
{
	class CommandBufferManager;
	class RenderPass;
}

namespace OmegaEngine
{
	// forward declerations
	class RenderQueue;
	struct RenderConfig;
	
	namespace Rendering
	{
		void render_objects(std::unique_ptr<RenderQueue>& render_queue,
			RenderConfig& render_config,
			VulkanAPI::RenderPass& renderpass,
			std::vector<RenderInterface::RenderableInfo>& renderables,
			std::unique_ptr<VulkanAPI::CommandBufferManager>& cmd_buffer_manager);
	}
}

