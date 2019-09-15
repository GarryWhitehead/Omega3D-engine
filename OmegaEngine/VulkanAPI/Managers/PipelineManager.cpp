#include "PipelineManager.h"

#include "Rendering/RenderableTypes/Mesh.h"
#include "Rendering/RenderableTypes/Shadow.h"
#include "Rendering/RenderableTypes/Skybox.h"

#include "VulkanAPI/Managers/ShaderManager.h"
#include "VulkanAPI/Types/BufferReflect.h"
#include "VulkanAPI/Types/ImageReflect.h"
#include "VulkanAPI/VkContext.h"

#include "Types/PBuildInfo.h"

namespace VulkanAPI
{

PipelineManager::PipelineManager()
{
}

PipelineManager::~PipelineManager()
{
}

bool PipelineManager::build(OmegaEngine::PBuildInfo& buildInfo, PStateInfo& state)
{
	BufferReflect bufferReflect;
	ImageReflect imageReflect;

	// load shaders and reflect
	for (auto& shader : buildInfo.shaderPaths)
	{
		GlslCompiler compiler(shader.second, shader.first);

		for (auto& define : buildInfo.shaderDefines)
		{
			compiler.addDefinition(define.first, define.second);
		}
		if (!compiler.compile(true))
		{
			LOGGER_ERROR("Unable to create shader bytecode with name %s.\n", shader.second.c_str());
			return false;
		}

		// now grab all the info we need from the shaaders for buffers and images....
		uint32_t* data = compiler.getData();
		size_t size = compiler.getSize();

		bufferReflect.prepare(data, size, state.descrLayout);
		imageReflect.prepare(data, size, state.descrLayout);

		// and pipline layouts and pipelines (push constants, etc.)
		PipelineLayout::reflect(data, size, state.pLayout);
		Pipeline::reflect(data, size, state.pipeline);

		context->getShaderManager().addShader(data, shader.first);
	}

	state.descrLayout.create(context->getDevice(), MAX_MATERIAL_SETS);

	// we only want to init the uniform buffer sets, the material image samplers will be created by the materials themselves
	for (auto& buffer : state->bufferLayout.layouts)
	{
		state->descriptorSet.init(vkInterface->getDevice(), state->descriptorLayout.getLayout(buffer.set),
		                          state->descriptorLayout.getDescriptorPool(), buffer.set);
	}

	// sort out the descriptor sets - as long as we have initilaised the VkBuffers, we don't need to have filled the buffers yet
	// material sets will be created and owned by the actual material - note: these will always be set ZERO
	auto& bufManager = context->getBufManager();

	for (auto& layout : state->bufferLayout.layouts)
	{
		// the shader must use these identifying names for uniform buffers -
		if (layout.name == "CameraUbo")
		{
			bufManager->enqueueDescrUpdate("Camera", &state.descrSet, layout.set,
			                                                    layout.binding, layout.type);
		}
		else if (layout.name == "Dynamic_StaticMeshUbo")
		{
			bufManager->enqueueDescrUpdate("Transform", &state.descrSet, layout.set,
			                                                    layout.binding, layout.type);
		}
		else if (layout.name == "Dynamic_SkinnedUbo")
		{
			bufManager->enqueueDescrUpdate("SkinnedTransform", &state.descrSet, layout.set,
			                                                    layout.binding, layout.type);
		}
	}

	// inform the texture manager the layout of textures associated with the mesh shader
	// TODO : automate this somehow rather than hard coded values
	auto& texManager = context->getTexManager();

	const uint8_t materialSet = 2;
	if (flags.mesh == StateMesh::Static)
	{
		texManager->bindTexturesToDescriptorLayout("Mesh", &state->descriptorLayout, materialSet);
	}
	else if (flags.mesh == StateMesh::Skinned)
	{
		texManager->bindTexturesToDescriptorLayout("SkinnedMesh", &state->descriptorLayout,
		                                                                 materialSet);
	}
	state->pipelineLayout.create(vkInterface->getDevice(), state->descriptorLayout.getLayout());

	// use stencil to fill in with ones where geometry is drawn
	state->pipeline.setStencilStateFrontAndBack(vk::CompareOp::eAlways, vk::StencilOp::eReplace,
	                                            vk::StencilOp::eReplace, vk::StencilOp::eReplace, 0xff, 0xff, 1);

	state.pipeline.setDepthState(VK_TRUE, VK_TRUE);
	state.pipeline.setRasterCullMode(vk::CullModeFlagBits::eBack);
	state.pipeline.setRasterFrontFace(vk::FrontFace::eCounterClockwise);
	state.pipeline.setTopology(flags.topology);
	state.pipeline.addColourAttachment(VK_FALSE, renderer->getFirstPass());
	state.pipeline.create(vkInterface->getDevice(), renderer->getFirstPass(), state->shader, state->pipelineLayout,
	                       VulkanAPI::PipelineType::Graphics);
}


}    // namespace VulkanAPI
