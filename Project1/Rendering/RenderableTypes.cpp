#include "RenderableTypes.h"
#include "Vulkan/BufferManager.h"
#include "Vulkan/PipelineInterface.h"

namespace OmegaEngine
{

	RenderableMesh::RenderableMesh(RenderTypes type) :
		RenderableBase(type)
	{
		// setup descriptor sets for each material

		// create ssbo buffer

		// build the graphics pipeline 

	}

	void RenderableMesh::build_graphics_pipeline(std::unique_ptr<PipelineInterface>& p_interface)
	{
		VulkanAPI::Pipeline pipeline;

		pipeline.set_topology(vk::PrimitiveTopology::eTriangleList);
	}

	void RenderableMesh::create_descriptor_set()
	{

	}

	void RenderableMesh::update_ssbo_buffer(std::unique_ptr<VulkanAPI::BufferManager>& buffer_man)
	{
		// transforms and camera hosted on ssbo - so will need updating

		// upload buffer to gpu
		buffer_man->add_buffer(ssbo, VulkanAPI::BufferMemoryType::Host, sizeof(MeshSSboBuffer));
	}
}