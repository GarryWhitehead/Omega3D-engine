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

bool PipelineManager::build(OmegaEngine::PBuildInfo& buildInfo, PStateInfo& state, RenderableInfo& renderable)
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

	// the user can state the max number of sets or the number will be defined from the reflect results.
	// defining the a greater number of sets is useful when you don't know the number of sets required at
	// compile time. This is the case for materials for example, wehere there is a set for each material type
	if (buildInfo.maxSetCount > 0)
	{
		state.descrLayout.create(context->getDevice(), buildInfo.maxSetCount);
	}
	else
	{
		state.descrLayout.create(context->getDevice());
	}

	// now initialise the descriptor sets, which use the layout prepared above.
	// In some cases not all sets will be defined here. For instance, material sets are created separately
	// and the information is stored in the renderable instance. We will create the material set now
	// if requied
	if (renderable.hasMaterial)
	{
		renderable.material.descrSet.init(context->getDevice());
		context->getTexManager().prepareSet(renderable.material.name, state.descrSet);
	}
	
	// prepare other descriptor sets
	state.descrSet.init(context->getDevice(), state.descrLayout);

	// writes descriptor sets - buffers must have been created prior to this
	context->getBufManager().prepareDescrSet(bufferReflect, state.descrSet);

	// also update descriptor sets for images
	context->getTexManager().prepareSet(imageReflect, state.descrSet);

	// with the descriptor layout prepared, we can create the pipeline layout. 
	// Pipeline layouts also contain push constant data which is retrieved through reflection
	state.pLayout.create(context->getDevice(), state.descrLayout);

	// now for the pipeline configuration details
	state->pipeline.setStencilStateFrontAndBack(vk::CompareOp::eAlways, vk::StencilOp::eReplace,
	                                            vk::StencilOp::eReplace, vk::StencilOp::eReplace, 0xff, 0xff, 1);

	state.pipeline.setDepthState(VK_TRUE, VK_TRUE);
	state.pipeline.setRasterCullMode(vk::CullModeFlagBits::eBack);
	state.pipeline.setRasterFrontFace(vk::FrontFace::eCounterClockwise);
	state.pipeline.setTopology(flags.topology);
	state.pipeline.addColourAttachment(VK_FALSE, renderer->getFirstPass());

	// and finally build the pipeline
	state.pipeline.create(context->getDevice(), renderer->getFirstPass(), state.shader, state.pLayout,
	                       VulkanAPI::PipelineType::Graphics);
}


}    // namespace VulkanAPI
