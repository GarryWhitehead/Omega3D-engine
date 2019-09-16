#include "Mesh.h"

#include "Managers/CameraManager.h"
#include "Managers/MaterialManager.h"
#include "Managers/MeshManager.h"
#include "Managers/TransformManager.h"

#include "Types/Object.h"
#include "Types/ComponentTypes.h"

#include "Rendering/RenderQueue.h"
#include "Rendering/Renderers/DeferredRenderer.h"

#include "Threading/ThreadPool.h"

#include "VulkanAPI/Managers/BufferManager.h"
#include "VulkanAPI/Managers/VkTextureManager.h"

#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/Sampler.h"

#include "Core/World.h"

namespace OmegaEngine
{

RenderableMesh::RenderableMesh()
    : RenderableBase(RenderTypes::StaticMesh)
{
}

void RenderableMesh::prepare(World& world, StaticMesh& mesh, PrimitiveMesh& primitive, Object& obj)
{
	VulkanAPI::VkTextureManager::TextureLayoutInfo layoutInfo;

	// get the material for this primitive mesh from the manager
	auto& matManager = world.getMatManager();
	auto& mat = matManager.getMaterial(primitive.materialId);

	// create the sorting key for this mesh
	sortKey = RenderQueue::createSortKey(RenderStage::First, primitive.materialId, RenderTypes::StaticMesh);

	// fill out the data which will be used for rendering
	// using a static cast here as faster than a reinterpret and we can be certain it ia mesh instance
	instanceData = new MeshInstance;
	MeshInstance* meshInstance = static_cast<MeshInstance*>(instanceData);

	// skinned ior non-skinned mesh?
	meshInstance->type = mesh.type;

	// queue and state type - opaque or transparent texture
	StateAlpha stateAlpha;
	if (mat.alphaMask == MaterialInfo::AlphaMode::Opaque)
	{
		queueType = QueueType::Opaque;
		stateAlpha = StateAlpha::Opaque;
	}
	else if (mat.alphaMask == MaterialInfo::AlphaMode::Blend)
	{
		queueType = QueueType::Transparent;
		stateAlpha = StateAlpha::Transparent;
	}

	meshInstance->transformDynamicOffset = obj.getComponent<TransformComponent>().dynamicUboOffset;

	// create the state - if a state with the same parameters has already been created, then will return
	// a pointer to this instance. Note: states should be created were possible before hand as they are expenisive
	// and will result in stuttering if created during the rendering loop.
	// This can be reduced by loading cached data which will be added at some point in the future.
	meshInstance->state = stateManager->createState(vkInterface, StateType::Mesh, mesh.topology, mesh.type, stateAlpha);

	// pointer to the mesh pipeline
	if (mesh.type == StateMesh::Static)
	{
		meshInstance->vertexBufferID = "StaticVertices";
		meshInstance->layoutInfoID = "StaticMesh";
	}
	else
	{
		meshInstance->vertexBufferID = "SkinnedVertices";
		meshInstance->layoutInfoID = "SkinnedMesh";
		meshInstance->skinnedDynamicOffset = obj.getComponent<SkinnedComponent>().dynamicUboOffset;
	}

	// index into the main buffer - this is the vertex offset plus the offset into the actual memory segment
	meshInstance->vertexOffset = mesh.vertexBufferOffset;
	meshInstance->indexOffset = mesh.indexBufferOffset;
	meshInstance->indexBufferID = "Indices";

	// per face indicies
	meshInstance->indexPrimitiveOffset = primitive.indexBase;
	meshInstance->indexPrimitiveCount = primitive.indexCount;

	// prepare the material data 
	meshInstance->pushBlock.prepare(mat);
}

void RenderableMesh::preparePStateInfo(StateId::StateFlags& flags)
{
	
	info.addShaderPath(PStateInfo::ShaderTarget::Vertex, "model/model.vert");
	info.addShaderDefinition("");
	info.setMaxSets(MAX_MATERIAL_SETS);
	info.setInitSetOnly(true);
	info.addUboTargets("CameraUbo", "Dynamic_StaticMeshUbo", "Dynamic_SkinnedUbo");
	
	info.setTopology(flags.topology);
}

void RenderableMesh::render(VulkanAPI::SecondaryCommandBuffer& cmdBuffer, void* instance)
{
	MeshInstance* instanceData = (MeshInstance*)instance;

	std::vector<uint32_t> dynamicOffsets{ instanceData->transformDynamicOffset };
	if (instanceData->type == StateMesh::Skinned)
	{
		dynamicOffsets.push_back(instanceData->skinnedDynamicOffset);
	}

	// merge the material set with the mesh ubo sets
	ProgramState* state = instanceData->state;
	std::vector<vk::DescriptorSet> materialSet = instanceData->descriptorSet.get();
	std::vector<vk::DescriptorSet> meshSet = state->descriptorSet.get();
	meshSet.insert(meshSet.end(), materialSet.begin(), materialSet.end());

	cmdBuffer.setViewport();
	cmdBuffer.setScissor();
	cmdBuffer.bindPipeline(state->pipeline);
	cmdBuffer.bindDynamicDescriptors(state->pipelineLayout, meshSet, VulkanAPI::PipelineType::Graphics, dynamicOffsets);
	cmdBuffer.bindPushBlock(state->pipelineLayout, vk::ShaderStageFlagBits::eFragment,
	                        sizeof(MeshInstance::MaterialPushBlock), &instanceData->materialPushBlock);

	vk::DeviceSize offset = { instanceData->vertexBuffer.offset };
	cmdBuffer.bindVertexBuffer(instanceData->vertexBuffer.buffer, offset);
	cmdBuffer.bindIndexBuffer(instanceData->indexBuffer.buffer,
	                          instanceData->indexBuffer.offset + (instanceData->indexOffset * sizeof(uint32_t)));
	cmdBuffer.drawIndexed(instanceData->indexPrimitiveCount, instanceData->indexPrimitiveOffset);
}  

}    // namespace OmegaEngine
