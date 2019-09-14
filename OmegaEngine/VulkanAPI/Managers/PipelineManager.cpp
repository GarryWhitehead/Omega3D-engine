#include "PipelineManager.h"

#include "Rendering/RenderableTypes/Mesh.h"
#include "Rendering/RenderableTypes/Shadow.h"
#include "Rendering/RenderableTypes/Skybox.h"

#include "VulkanAPI/Managers/ShaderManager.h"
#include "VulkanAPI/Types/BufferReflect.h"
#include "VulkanAPI/Types/ImageReflect.h"

#include "Types/PStateInfo.h"

#include "spirv_cross.hpp"

namespace VulkanAPI
{

PipelineManager::PipelineManager()
{
}

PipelineManager::~PipelineManager()
{
}

bool PipelineManager::build(const PStateInfo& pInfo)
{
	BufferReflect bufferReflect;
	ImageReflect imageReflect;

	// load shaders and reflect
	for (auto& shader : pInfo.shaderPaths)
	{
		GlslCompiler compiler(shader.second, shader.first);

		for (auto& define : pInfo.shaderDefines)
		{
			compiler.addDefinition(define.first, define.second);
		}
		if (!compiler.compile(true))
		{
			LOGGER_ERROR("Unable to create shader bytecode with name %s.\n", shader.second.c_str());
			return false;
		}

		// now grab all the info we need from the shaaders
		uint32_t* data = compiler.getData();
		size_t size = compiler.getDataSize();

		bufferReflect.reflect(data, size, descrLayout);
		imageReflect.reflect(data, size, descrLayout);

		vkInterface.getShaderManager().addShader(data, shader.first);
	}

	// get pipeline layout and vertedx attributes by reflection of shader
	state->shader.imageReflection(state->descriptorLayout, state->imageLayout);
	state->shader.bufferReflection(state->descriptorLayout, state->bufferLayout);
	state->descriptorLayout.create(vkInterface->getDevice(), MAX_MATERIAL_SETS);

	// we only want to init the uniform buffer sets, the material image samplers will be created by the materials themselves
	for (auto& buffer : state->bufferLayout.layouts)
	{
		state->descriptorSet.init(vkInterface->getDevice(), state->descriptorLayout.getLayout(buffer.set),
		                          state->descriptorLayout.getDescriptorPool(), buffer.set);
	}

	// sort out the descriptor sets - as long as we have initilaised the VkBuffers, we don't need to have filled the buffers yet
	// material sets will be created and owned by the actual material - note: these will always be set ZERO
	for (auto& layout : state->bufferLayout.layouts)
	{
		// the shader must use these identifying names for uniform buffers -
		if (layout.name == "CameraUbo")
		{
			vkInterface->getBufferManager()->enqueueDescrUpdate("Camera", &state->descriptorSet, layout.set,
			                                                    layout.binding, layout.type);
		}
		else if (layout.name == "Dynamic_StaticMeshUbo")
		{
			vkInterface->getBufferManager()->enqueueDescrUpdate("Transform", &state->descriptorSet, layout.set,
			                                                    layout.binding, layout.type);
		}
		else if (layout.name == "Dynamic_SkinnedUbo")
		{
			vkInterface->getBufferManager()->enqueueDescrUpdate("SkinnedTransform", &state->descriptorSet, layout.set,
			                                                    layout.binding, layout.type);
		}
	}

	// inform the texture manager the layout of textures associated with the mesh shader
	// TODO : automate this somehow rather than hard coded values
	const uint8_t materialSet = 2;
	if (flags.mesh == StateMesh::Static)
	{
		vkInterface->gettextureManager()->bindTexturesToDescriptorLayout("Mesh", &state->descriptorLayout, materialSet);
	}
	else if (flags.mesh == StateMesh::Skinned)
	{
		vkInterface->gettextureManager()->bindTexturesToDescriptorLayout("SkinnedMesh", &state->descriptorLayout,
		                                                                 materialSet);
	}

	state->shader.pipelineLayoutReflect(state->pipelineLayout);
	state->pipelineLayout.create(vkInterface->getDevice(), state->descriptorLayout.getLayout());

	// create the graphics pipeline
	state->shader.pipelineReflection(state->pipeline);

	// use stencil to fill in with ones where geometry is drawn
	state->pipeline.setStencilStateFrontAndBack(vk::CompareOp::eAlways, vk::StencilOp::eReplace,
	                                            vk::StencilOp::eReplace, vk::StencilOp::eReplace, 0xff, 0xff, 1);

	state->pipeline.setDepthState(VK_TRUE, VK_TRUE);
	state->pipeline.setRasterCullMode(vk::CullModeFlagBits::eBack);
	state->pipeline.setRasterFrontFace(vk::FrontFace::eCounterClockwise);
	state->pipeline.setTopology(flags.topology);
	state->pipeline.addColourAttachment(VK_FALSE, renderer->getFirstPass());
	state->pipeline.create(vkInterface->getDevice(), renderer->getFirstPass(), state->shader, state->pipelineLayout,
	                       VulkanAPI::PipelineType::Graphics);
}

void Shader::reflectLayout(uint32_t* data, size_t dataSize, PipelineLayout& layout)
{
	// reflect for each stage that has been setup
	for (uint8_t i = 0; i < (uint8_t)StageType::Count; ++i)
	{
		if (!currentStages[i])
		{
			continue;
		}

		spirv_cross::Compiler compiler(data[i].data(), data[i].size() / sizeof(uint32_t));

		auto shaderResources = compiler.get_shader_resources();

		// get push constants struct sizes if any
		if (!shaderResources.push_constant_buffers.empty())
		{
			layout.add_push_constant((StageType)i, (uint32_t)compiler.get_declared_struct_size(compiler.get_type(
			                                           shaderResources.push_constant_buffers.front().base_type_id)));
		}
	}
}

void PipelineManager::reflectPipeline(uint32_t* data, size_t dataSize, Pipeline& pipeline)
{
	if (!currentStages[(int)StageType::Vertex])
	{
		LOGGER_ERROR("Unable to reflect pipeline as vertex shader has not been initialised.");
	}

	spirv_cross::Compiler compiler(data, dataSize / sizeof(uint32_t));

	auto shaderResources = compiler.get_shader_resources();

	// get the number of input and output stages
	for (auto& input : shaderResources.stage_inputs)
	{
		uint32_t location = compiler.get_decoration(input.id, spv::DecorationLocation);
		auto& base_type = compiler.get_type(input.base_type_id);
		auto& member = compiler.get_type(base_type.self);

		if (member.vecsize && member.columns == 1)
		{
			std::tuple<vk::Format, uint32_t> info = getTypeFormat(member.width, member.vecsize, member.basetype);
			pipeline.addVertexInput(location, std::get<0>(info), std::get<1>(info));
		}
	}
	/*for (auto& output : shaderResources.stage_outputs) {

			uint32_t location = compiler.get_decoration(output.id, spv::DecorationLocation);
		}*/
}
}    // namespace VulkanAPI
