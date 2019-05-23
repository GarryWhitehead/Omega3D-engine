#include "Mesh.h"
#include "Vulkan/BufferManager.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/VkTextureManager.h"
#include "Managers/CameraManager.h"
#include "Managers/MeshManager.h"
#include "Managers/TextureManager.h"
#include "Managers/MaterialManager.h"
#include "Managers/MeshManager.h"
#include "Managers/TransformManager.h"
#include "Rendering/RenderInterface.h"
#include "Rendering/Renderers/DeferredRenderer.h"
#include "Rendering/RenderQueue.h"
#include "Rendering/RenderCommon.h"
#include "Managers/ComponentInterface.h"
#include "Threading/ThreadPool.h"

namespace OmegaEngine
{

	RenderableMesh::RenderableMesh(vk::Device& device,
		std::unique_ptr<ComponentInterface>& componentInterface,
		std::unique_ptr<VulkanAPI::BufferManager>& bufferManager,
		std::unique_ptr<VulkanAPI::VkTextureManager>& textureManager,
		MeshManager::StaticMesh mesh,
		MeshManager::PrimitiveMesh primitive,
		Object& obj,
		RenderInterface* renderInterface) :
		RenderableBase(RenderTypes::StaticMesh)
	{
		VulkanAPI::VkTextureManager::TextureLayoutInfo layoutInfo;

		// get the material for this primitive mesh from the manager
		auto& materialManager = componentInterface->getManager<MaterialManager>();
		auto& mat = materialManager.get(primitive.materialId);

		// create the sorting key for this mesh
		sortKey = RenderQueue::createSortKey(RenderStage::First, primitive.materialId, RenderTypes::StaticMesh);

		// fill out the data which will be used for rendering
		instanceData = new MeshInstance;
		MeshInstance* meshInstance = reinterpret_cast<MeshInstance*>(instanceData);

		// skinned ior non-skinned mesh?
		meshInstance->type = mesh.type;

		// queue type - opaque or transparent texture
		if (mat.factors.alphaMask == MaterialInfo::AlphaMode::Opaque)
		{
			queueType = QueueType::Opaque;
		}
		else if (mat.factors.alphaMask == MaterialInfo::AlphaMode::Blend)
		{
			queueType = QueueType::Transparent;
		}

		auto& transformManagerager = componentInterface->getManager<TransformManager>();
		meshInstance->transformDynamicOffset = transformManagerager.getTransformOffset(obj.getId());

		// pointer to the mesh pipeline
		if (mesh.type == MeshManager::MeshType::Static) {
			meshInstance->state = renderInterface->getRenderPipeline(RenderTypes::StaticMesh).get();
			meshInstance->vertexBuffer = bufferManager->getBuffer("StaticVertices");
			layoutInfo = textureManager->getTextureDescriptorLayout("Mesh");
		}
		else {
			meshInstance->state = renderInterface->getRenderPipeline(RenderTypes::SkinnedMesh).get();
			meshInstance->vertexBuffer = bufferManager->getBuffer("SkinnedVertices");
			layoutInfo = textureManager->getTextureDescriptorLayout("SkinnedMesh");
			meshInstance->skinnedDynamicOffset = transformManagerager.getSkinnedOffset(obj.getId());
		}

		// index into the main buffer - this is the vertex offset plus the offset into the actual memory segment
		meshInstance->vertexOffset = mesh.vertexBufferOffset;
		meshInstance->indexOffset = mesh.indexBufferOffset;
		meshInstance->indexBuffer = bufferManager->getBuffer("Indices");
		
		// per face indicies
		meshInstance->indexPrimitiveOffset = primitive.indexBase;
		meshInstance->indexPrimitiveCount = primitive.indexCount;
			
		meshInstance->descriptorSet.init(device, *layoutInfo.layout, layoutInfo.setValue); 
		textureManager->update_material_descriptorSet(meshInstance->descriptorSet, mat.name, layoutInfo.setValue);

		// material push block
		meshInstance->materialPushBlock.baseColorFactor = mat.factors.baseColour;
		meshInstance->materialPushBlock.metallicFactor = mat.factors.metallic;
		meshInstance->materialPushBlock.roughnessFactor = mat.factors.roughness;
		meshInstance->materialPushBlock.emissiveFactor = mat.factors.emissive;
		meshInstance->materialPushBlock.specularFactor = mat.factors.specular;
		meshInstance->materialPushBlock.diffuseFactor = OEMaths::vec3f{ mat.factors.diffuse.getX(), mat.factors.diffuse.getY(), mat.factors.diffuse.getZ() };
		meshInstance->materialPushBlock.alphaMask = (float)mat.factors.alphaMask;
		meshInstance->materialPushBlock.alphaMaskCutoff = mat.factors.alphaMaskCutOff;
		meshInstance->materialPushBlock.haveBaseColourMap = mat.textureState[(int)PbrMaterials::BaseColor] ? 1 : 0;
		meshInstance->materialPushBlock.haveMrMap = mat.textureState[(int)PbrMaterials::MetallicRoughness] ? 1 : 0;
		meshInstance->materialPushBlock.haveNormalMap = mat.textureState[(int)PbrMaterials::Normal] ? 1 : 0;
		meshInstance->materialPushBlock.haveAoMap = mat.textureState[(int)PbrMaterials::Occlusion] ? 1 : 0;
		meshInstance->materialPushBlock.haveEmissiveMap = mat.textureState[(int)PbrMaterials::Emissive] ? 1 : 0;
		meshInstance->materialPushBlock.usingSpecularGlossiness = mat.usingSpecularGlossiness ? 1 : 0;
		meshInstance->materialPushBlock.baseColourUvSet = mat.uvSets.baseColour;
		meshInstance->materialPushBlock.metallicRoughnessUvSet = mat.uvSets.metallicRoughness;
		meshInstance->materialPushBlock.normalUvSet = mat.uvSets.normal;
		meshInstance->materialPushBlock.occlusionUvSet = mat.uvSets.occlusion;
		meshInstance->materialPushBlock.emissiveUvSet = mat.uvSets.emissive;

		if (meshInstance->materialPushBlock.usingSpecularGlossiness) {
			meshInstance->materialPushBlock.metallicRoughnessUvSet = mat.uvSets.specularGlossiness;
			meshInstance->materialPushBlock.baseColourUvSet = mat.uvSets.diffuse;
		}
	}
	
	void RenderableMesh::createMeshPipeline(vk::Device& device, 
										std::unique_ptr<RendererBase>& renderer, 
										std::unique_ptr<VulkanAPI::BufferManager>& bufferManager,
										std::unique_ptr<VulkanAPI::VkTextureManager>& textureManager,
										MeshManager::MeshType type,
										std::unique_ptr<ProgramState>& state)
	{
		// load shaders
		if (type == MeshManager::MeshType::Static) {
			if (!state.shader.add(device, "model/model-vert.spv", VulkanAPI::StageType::Vertex, "model/model-frag.spv", VulkanAPI::StageType::Fragment)) {
				LOGGER_ERROR("Unable to create static model shaders.");
			}
		}
		else if (type == MeshManager::MeshType::Skinned) {
			if (!state.shader.add(device, "model/model_skinned-vert.spv", VulkanAPI::StageType::Vertex, "model/model-frag.spv", VulkanAPI::StageType::Fragment)) {
				LOGGER_ERROR("Unable to create skinned model shaders.");
			}
		}

		// get pipeline layout and vertedx attributes by reflection of shader
		state.shader.imageReflection(state.descriptorLayout, state.imageLayout);
		state.shader.bufferReflection(state.descriptorLayout, state.bufferLayout);
		state.descriptorLayout.create(device, MAX_MATERIAL_SETS);

		// we only want to init the uniform buffer sets, the material image samplers will be created by the materials themselves
		for (auto& buffer : state.bufferLayout) {
			state.descriptorSet.init(device, state.descriptorLayout.getLayout(buffer.set), state.descriptorLayout.getDescriptorPool(), buffer.set);
		}

		// sort out the descriptor sets - as long as we have initilaised the VkBuffers, we don't need to have filled the buffers yet
		// material sets will be created and owned by the actual material - note: these will always be set ZERO
		for (auto& layout : state.bufferLayout) {
			
			// the shader must use these identifying names for uniform buffers -
			if (layout.name == "CameraUbo") {
				bufferManager->enqueueDescrUpdate("Camera", &state.descriptorSet, layout.set, layout.binding, layout.type);
			}
			else if (layout.name == "Dynamic_StaticMeshUbo") {
				bufferManager->enqueueDescrUpdate("Transform", &state.descriptorSet, layout.set, layout.binding, layout.type);
			}
			else if (layout.name == "Dynamic_SkinnedUbo") {
				bufferManager->enqueueDescrUpdate("SkinnedTransform", &state.descriptorSet, layout.set, layout.binding, layout.type);
			}
		}

		// inform the texture manager the layout of textures associated with the mesh shader
		// TODO : automate this somehow rather than hard coded values
		const uint8_t materialSet = 2;
		if (type == MeshManager::MeshType::Static) {
			textureManager->bindTexturesToDescriptorLayout("Mesh", &state.descriptorLayout, materialSet);
		}
		else if (type == MeshManager::MeshType::Skinned) {
			textureManager->bindTexturesToDescriptorLayout("SkinnedMesh", &state.descriptorLayout, materialSet);
		}

		state.shader.pipelineLayoutReflect(state.pipelineLayout);
		state.pipelineLayout.create(device, state.descriptorLayout.getLayout());

		// create the graphics pipeline
		state.shader.pipelineReflection(state.pipeline);

		state.pipeline.setDepthState(VK_TRUE, VK_TRUE);
		state.pipeline.setRasterCullMode(vk::CullModeFlagBits::eBack);
		state.pipeline.setRasterFrontFace(vk::FrontFace::eClockwise);
		state.pipeline.setTopology(vk::PrimitiveTopology::eTriangleList);
		state.pipeline.addColourAttachment(VK_FALSE, renderer->getFirstPass());
		state.pipeline.create(device, renderer->getFirstPass(), state.shader, state.pipelineLayout, VulkanAPI::PipelineType::Graphics);
	}

	void RenderableMesh::render(VulkanAPI::SecondaryCommandBuffer& cmdBuffer, 
								void* instance)
	{
		MeshInstance* instanceData = (MeshInstance*)instance;

		std::vector<uint32_t> dynamicOffsets { instanceData->transformDynamicOffset };
		if (instanceData->type == MeshManager::MeshType::Skinned) {
			dynamicOffsets.push_back(instanceData->skinnedDynamicOffset);
		}

		// merge the material set with the mesh ubo sets
		ProgramState* state = instanceData->state;
		std::vector<vk::DescriptorSet> materialSet = instanceData->descriptorSet.get();
		std::vector<vk::DescriptorSet> meshSet = state.descriptorSet.get();
		mesh_set.insert(meshSet.end(), materialSet.begin(), materialSet.end());

		cmdBuffer.setViewport();
		cmdBuffer.setScissor();
		cmdBuffer.bindPipeline(state.pipeline);
		cmdBuffer.bindDynamicDescriptors(state.pipelineLayout, meshSet, VulkanAPI::PipelineType::Graphics, dynamicOffsets);
		cmdBuffer.bindPushBlock(state.pipelineLayout, vk::ShaderStageFlagBits::eFragment, sizeof(MeshInstance::MaterialPushBlock), &instanceData->materialPushBlock);

		vk::DeviceSize offset = { instanceData->vertexBuffer.offset };
		cmdBuffer.bindVertexBuffer(instanceData->vertexBuffer.buffer, offset);
		cmdBuffer.bindIndexBuffer(instanceData->indexBuffer.buffer, instanceData->indexBuffer.offset + instanceData->indexOffset + instanceData->indexPrimitiveOffset);
		cmdBuffer.drawIndexed(instanceData->indexPrimitiveCount);
	}

}