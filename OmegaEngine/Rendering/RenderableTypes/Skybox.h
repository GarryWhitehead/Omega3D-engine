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
	class Texture;
}

namespace OmegaEngine
{
	struct SkyboxComponent;
	
	class RenderableSkybox : public RenderableBase
	{

	public:

		struct SkyboxInstance
		{
			// pipeline
			ProgramState* state;
			
			// vertex and index buffer memory info for the cube
			VulkanAPI::Buffer vertex_buffer;
			VulkanAPI::Buffer index_buffer;
			uint32_t index_count = 0;

			// the factor which the skybox will be blurred by
			float blur_factor = 0.0f;
		};

		RenderableSkybox(RenderInterface* render_interface, SkyboxComponent& component);
		~RenderableSkybox();
		
		static void create_skybox_pipeline(vk::Device& device,
										std::unique_ptr<RendererBase>& renderer, 
										std::unique_ptr<VulkanAPI::BufferManager>& buffer_manager,
										std::unique_ptr<VulkanAPI::VkTextureManager>& texture_manager,
										std::unique_ptr<ProgramState>& state);
		
		static void create_skybox_pass(VulkanAPI::RenderPass& renderpass, VulkanAPI::Texture& image, VulkanAPI::Texture& depth_image,
			vk::Device& device, vk::PhysicalDevice& gpu, const uint32_t width, const uint32_t height);

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

