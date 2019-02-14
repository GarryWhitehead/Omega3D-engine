#pragma once
#include "RenderableBase.h"
#include "Vulkan/MemoryAllocator.h"
#include "Vulkan/Descriptors.h"
#include "Vulkan/DataTypes/PushBlocks.h"

// forward decleartions
namespace VulkanAPI
{
	class Sampler;
	class CommandBuffer;
}

namespace OmegaEngine
{
	// forward declerations
	class PipelineInterface;
	class DeferredRenderer;
	class ComponentInterface;
	class ThreadPool;
	struct RenderPipeline;
	class Object;

	// renderable object types
	class RenderableMesh : public RenderableBase
	{

	public:

		// render info that will be used to draw this mesh 
		struct MeshInstance
		{
			// face indicies data
			uint32_t index_buffer_offset; // index into large buffer
			uint32_t index_sub_offset;	// this equates to buffer_offset + sub-offset
			uint32_t index_count;

			// all material data required to draw
			// storing this material data in two places for threading purposes. We could get data races
			// if we start calling back to material manager whilst is updating in a different thread
			struct MaterialPushBlock
			{
				OEMaths::vec3f emissive;
				float specularGlossiness;
				float baseColour;
				float roughness;
				float diffuse;
				float metallic;
				float specular;
				float alphaMask;
				float alphaMaskCutOff;
			} material_push_block;

			// vulkan stuff for material textures
			VulkanAPI::DescriptorSet descr_set;
			VulkanAPI::Sampler sampler;

			// offset into transform buffer
			uint32_t transform_dynamic_offset = 0;
			uint32_t skinned_dynamic_offset = 0;
		};
		
		RenderableMesh(std::unique_ptr<ComponentInterface>& comp_interface, Object& obj);

		void render(VulkanAPI::CommandBuffer& cmd_buffer, 
					MeshInstance* instance_data,
					std::unique_ptr<ComponentInterface>& component_interface,
					RenderInterface* render_interface) override;

		static RenderPipeline create_mesh_pipeline(vk::Device device, 
													std::unique_ptr<DeferredRenderer>& renderer, 
													std::unique_ptr<ComponentInterface>& component_interface);	

	private:

		// render instance data for all sub-meshes associated with this mesh
		std::vector<MeshInstance> mesh_render_info;

		// offsets into the mapped buffer
		uint32_t vertex_buffer_offset;
		uint32_t index_buffer_offset;
	};
}