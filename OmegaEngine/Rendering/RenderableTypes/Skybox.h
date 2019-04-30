#pragma once
#include "RenderableBase.h"

namespace OmegaEngine
{

	class RenderableSkybox : public RenderableBase
	{

	public:

		RenderableSkybox();
		~RenderableSkybox();
		
		static void create_skybox_pipeline(vk::Device& device,
										std::unique_ptr<RendererBase>& renderer, 
										std::unique_ptr<VulkanAPI::BufferManager>& buffer_manager,
										std::unique_ptr<VulkanAPI::VkTextureManager>& texture_manager,
										std::unique_ptr<RenderInterface::ProgramState>& state);
	private:

	};

}

