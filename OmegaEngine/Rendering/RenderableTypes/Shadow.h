#pragma once
#include "RenderableBase.h"
#include "Rendering/RenderInterface.h"
#include "Managers/MeshManager.h"
#include "Rendering/RenderableTypes/Mesh.h"

//forward declerations
namespace VulkanAPI
{
	class BufferManager;
	class SecondaryCommandBuffer;
	class Texture;
}

namespace OmegaEngine
{	
	struct ShadowComponent;
	
	class RenderableShadow : public RenderableBase
	{

	public:

		struct ShadowInstance
		{
			// pipeline
			ProgramState* state;
			
			// vertex and index buffer memory info for the cube
			VulkanAPI::Buffer vertex_buffer;
			VulkanAPI::Buffer index_buffer;
			uint32_t index_count = 0;

			uint32_t vertex_offset = 0;
			uint32_t index_offset = 0;

			uint32_t transform_dynamic_offset = 0;
			uint32_t skinned_dynamic_offset = 0;
			MeshManager::MeshType mesh_type;

			float bias_constant = 0.0f;
			float bias_clamp = 0.0f;
			float bias_slope = 0.0f;
		};

		RenderableShadow(RenderInterface* render_interface, ShadowComponent& component, RenderableMesh::MeshInstance* mesh_instance);
		~RenderableShadow();
		
		static void create_shadow_pipeline(vk::Device& device,
										std::unique_ptr<RendererBase>& renderer, 
										std::unique_ptr<VulkanAPI::BufferManager>& buffer_manager,
										std::unique_ptr<ProgramState>& state,
										MeshManager::MeshType type);
		
		static void create_shadow_pass(VulkanAPI::RenderPass& renderpass, 
										VulkanAPI::Texture& image,
										vk::Device& device, vk::PhysicalDevice& gpu, 
										const vk::Format format, 
										const uint32_t width, const uint32_t height);

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
