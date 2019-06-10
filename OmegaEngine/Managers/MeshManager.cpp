#include "MeshManager.h"
#include "Utility/logger.h"
#include "Utility/GeneralUtil.h"
#include "Engine/Omega_Global.h"
#include "Vulkan/BufferManager.h"
#include "Managers/EventManager.h"
#include "ObjectInterface/ObjectManager.h"
#include "ObjectInterface/Object.h"
#include "ObjectInterface/ComponentTypes.h"
#include "Models/ModelMesh.h"
#include "OEMaths/OEMaths_transform.h"

namespace OmegaEngine
{

	MeshManager::MeshManager()
	{
	}


	MeshManager::~MeshManager()
	{
	}

	void MeshManager::addComponentToManager(MeshComponent& component, Object& object)
	{
		StaticMesh mesh;
		auto vertexData = component.mesh->getVertices();

		// copy data from model into the manager
		if (!component.mesh->hasSkin())
		{
			mesh.vertexBufferOffset = staticVertices.size();

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
			mesh.vertexBufferOffset = skinnedVertices.size();

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
		auto& modelIndices = component.mesh->getIndices();

		uint32_t indexOffset = indices.size();
		mesh.indexBufferOffset = indexOffset;

		// simple copy from the model data to the manager
		indices.resize(indexOffset + modelIndices.size());
		std::copy(modelIndices.begin(), modelIndices.end(), indices.begin() + indexOffset);

		// and the primitive data
		auto& modelPrimitives = component.mesh->getPrimitives();

		for (auto& modelPrimitive : modelPrimitives)
		{
			PrimitiveMesh primitive;
			primitive.indexBase = modelPrimitive.indexBase + indexOffset;
			primitive.indexCount = modelPrimitive.indexCount;
			mesh.primitives.emplace_back(primitive);
		}
	}

	void MeshManager::updateFrame(double time, double dt, std::unique_ptr<ObjectManager>& objectManager, ComponentInterface* componentInterface)
	{
		if (isDirty) 
		{	
			// static and skinned meshes if any
			if (!staticVertices.empty()) 
			{
				VulkanAPI::BufferUpdateEvent event{ "StaticVertices", staticVertices.data(), staticVertices.size() * sizeof(Vertex), VulkanAPI::MemoryUsage::VK_BUFFER_STATIC };
				Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);
			}
			if (!skinnedVertices.empty()) 
			{
				VulkanAPI::BufferUpdateEvent event{ "SkinnedVertices", skinnedVertices.data(), skinnedVertices.size() * sizeof(SkinnedVertex), VulkanAPI::MemoryUsage::VK_BUFFER_STATIC };
				Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);
			}
			
			// and the indices....
			VulkanAPI::BufferUpdateEvent event{ "Indices", indices.data(), indices.size() * sizeof(uint32_t), VulkanAPI::MemoryUsage::VK_BUFFER_STATIC };
			Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);

			isDirty = false;
		}
	}
}
