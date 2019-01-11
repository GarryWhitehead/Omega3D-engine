#include "RenderableTypes.h"
#include "Vulkan/BufferManager.h"
#include "Vulkan/Pipeline.h"
#include "Rendering/RenderInterface.h"

namespace OmegaEngine
{

	RenderableMesh::RenderableMesh(RenderTypes type) :
		RenderableBase(type)
	{
	}
	
	RenderPipeline create_mesh_pipeline(vk::Device device)
	{
		RenderPipeline pipeline_info;
		
		// load shaders
		pipeline_info.shader.add(device, "model.vert", VulkanAPI::StageType::Vertex, "model.frag", VulkanAPI::StageType::Fragment);

		// get pipeline layout and vertedx attributes by reflection of shader
		std::vector<VulkanAPI::ShaderBufferLayout> buffer_layout;
		std::vector<VulkanAPI::ShaderImageLayout> image_layout;
		pipeline_info.shader.reflection(VulkanAPI::StageType::Vertex, *pipeline_info.descr_layout, pipeline_info.pl_layout, pipeline_info.pipeline, image_layout, buffer_layout);
		pipeline_info.shader.reflection(VulkanAPI::StageType::Fragment, *pipeline_info.descr_layout, pipeline_info.pl_layout, pipeline_info.pipeline, image_layout, buffer_layout);

		// descriptor sets

		// finally create the actuasl graphics pipeline

	}

	void RenderableMesh::update_ssbo_buffer(std::unique_ptr<VulkanAPI::BufferManager>& buffer_man)
	{
		// transforms and camera hosted on ssbo - so will need updating

		// upload buffer to gpu
		buffer_man->add_buffer(ssbo, VulkanAPI::BufferMemoryType::Host, sizeof(MeshSSboBuffer));
	}
}