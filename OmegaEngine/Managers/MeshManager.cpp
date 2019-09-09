#include "MeshManager.h"

#include "Core/Omega_Global.h"

#include "Managers/EventManager.h"
#include "Managers/ObjectManager.h"

#include "Models/ModelMesh.h"

#include "OEMaths/OEMaths_transform.h"

#include "Types/ComponentTypes.h"

#include "Utility/GeneralUtil.h"
#include "Utility/logger.h"

#include "VulkanAPI/BufferManager.h"

#include "Rendering/ProgramStateManager.h"

namespace OmegaEngine
{

MeshManager::MeshManager()
{
	// for performance purposes
	meshBuffer.reserve(MESH_INIT_CONTAINER_SIZE);
	staticVertices.reserve(1000000);		// these numbers need evaluating
	skinnedVertices.reserve(1000000);
	indices.reserve(1000000);
}

MeshManager::~MeshManager()
{
}

void MeshManager::linkMaterialWithMesh(MeshComponent* meshComponent, MaterialComponent* materialComponent)
{
	uint32_t meshIndex = meshComponent->index;

	// for now, assume all primitives have the same material
	for (auto& primitive : meshBuffer[meshIndex].primitives)
	{
		primitive.materialId = materialComponent->offset;
	}
}

void MeshManager::addMesh(std::unique_ptr<ModelMesh>& mesh, Object& obj)
{
	StaticMesh newMesh;
	auto vertexData = mesh->vertices;

	// copy data from model into the manager
	if (!mesh->skinned)
	{
		newMesh.type = StateMesh::Static;
		newMesh.vertexBufferOffset = static_cast<uint32_t>(staticVertices.size());

		for (auto& vertex : vertexData)
		{
			Vertex vert;
			vert.normal = vertex.normal;
			vert.position = vertex.position;
			vert.uv0 = vertex.uv0;
			vert.uv1 = vertex.uv1;

			staticVertices.emplace_back(vert);
		}
	}
	else
	{
		newMesh.type = StateMesh::Skinned;
		newMesh.vertexBufferOffset = static_cast<uint32_t>(skinnedVertices.size());

		for (auto& vertex : vertexData)
		{
			SkinnedVertex vert;
			vert.normal = vertex.normal;
			vert.position = vertex.position;
			vert.uv0 = vertex.uv0;
			vert.uv1 = vertex.uv1;
			vert.weight = vertex.weight;
			vert.joint = vertex.joint;

			skinnedVertices.emplace_back(vert);
		}
	}

	// and now the indices
	auto& modelIndices = mesh->indices;

	size_t indexOffset = indices.size();
	newMesh.indexBufferOffset = indexOffset;

	// simple copy from the model data to the manager
	for (size_t i = 0; i < modelIndices.size(); ++i)
	{
		indices.emplace_back(modelIndices[i] + mesh.vertexBufferOffset);
	}

	// and the primitive data
	auto& modelPrimitives = mesh->primitives;

	for (auto& modelPrimitive : modelPrimitives)
	{
		PrimitiveMesh primitive;
		primitive.indexBase = modelPrimitive.indexBase;
		primitive.indexCount = modelPrimitive.indexCount;
		primitive.materialId = modelPrimitive.materialId + component->materialBufferOffset;
		newMesh.primitives.emplace_back(primitive);
	}

	newMesh.topology = mesh->topology;

	meshBuffer.emplace_back(newMesh);

	// store the buffer index in the mesh component
	MeshComponent mcomp;
	mcomp.setIndex(index);
	obj.addComponent<MeshComponent>(mcomp);
}


void MeshManager::updateFrame()
{
	if (isDirty)
	{
		// static and skinned meshes if any
		if (!staticVertices.empty())
		{
			VulkanAPI::BufferUpdateEvent event{ "StaticVertices", staticVertices.data(),
				                                staticVertices.size() * sizeof(Vertex),
				                                VulkanAPI::MemoryUsage::VK_BUFFER_STATIC };
			Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);
		}
		if (!skinnedVertices.empty())
		{
			VulkanAPI::BufferUpdateEvent event{ "SkinnedVertices", skinnedVertices.data(),
				                                skinnedVertices.size() * sizeof(SkinnedVertex),
				                                VulkanAPI::MemoryUsage::VK_BUFFER_STATIC };
			Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);
		}

		// and the indices....
		VulkanAPI::BufferUpdateEvent event{ "Indices", indices.data(), indices.size() * sizeof(uint32_t),
			                                VulkanAPI::MemoryUsage::VK_BUFFER_STATIC };
		Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);

		isDirty = false;
	}
}
}    // namespace OmegaEngine
