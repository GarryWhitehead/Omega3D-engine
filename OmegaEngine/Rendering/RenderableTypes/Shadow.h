#pragma once
#include "RenderableBase.h"
#include "Rendering/RenderInterface.h"
#include "Vulkan/BufferManager.h"

//forward declerations
namespace VulkanAPI
{
	class BufferManager;
	class VkTextureManager;
	class SecondaryCommandBuffer;
}

namespace OmegaEngine
{
	
	class RenderableShadow : public RenderableBase
	{

	public:

		struct ShadowInstance
		{
			// pipeline
			RenderInterface::ProgramState* state;
			
			// vertex and index buffer memory info for the cube
			VulkanAPI::Buffer vertex_buffer;
			VulkanAPI::Buffer index_buffer;
			uint32_t index_count = 0;

		};

		RenderableSkybox(RenderInterface* render_interface);
		~RenderableSkybox();
		
		static void create_shadow_pipeline(vk::Device& device,
										std::unique_ptr<RendererBase>& renderer, 
										std::unique_ptr<VulkanAPI::BufferManager>& buffer_manager,
										std::unique_ptr<VulkanAPI::VkTextureManager>& texture_manager,
										std::unique_ptr<RenderInterface::ProgramState>& state);
		
		// used to get the address of this instance
		void* get_handle() override
		{
			return this;
		}

		void render(VulkanAPI::SecondaryCommandBuffer& cmd_buffer, 
					void* instance_data) override;
					
	private:

	};

}
