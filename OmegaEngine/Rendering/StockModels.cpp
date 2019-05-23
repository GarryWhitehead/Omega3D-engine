#include "StockModels.h"
#include "Vulkan/BufferManager.h"
#include "Engine/Omega_Global.h"
#include "Managers/EventManager.h"

namespace OmegaEngine
{
	namespace RenderUtil
	{

		CubeModel::CubeModel()
		{
			// vertex data
			VulkanAPI::BufferUpdateEvent vertexEvent{ "CubeModelVertices", vertices.data(), vertices.size() * sizeof(float), VulkanAPI::MemoryUsage::VK_BUFFER_STATIC };
			Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(vertexEvent);

			// index data
			VulkanAPI::BufferUpdateEvent indexEvent{ "CubeModelIndices", indices.data(), indices.size() * sizeof(uint32_t), VulkanAPI::MemoryUsage::VK_BUFFER_STATIC };
			Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(indexEvent);
		}

		CubeModel::~CubeModel()
		{
		}

		PlaneModel::PlaneModel()
		{
			// vertices
			std::vector<Vertex> vertices = {
			{ { 1.0f, 1.0f },{ 1.0f, 1.0f, 0.0f },{ 0.0f, 0.0f, 0.0f }},
			{ { 0.0f, 1.0f },{ 0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f, 0.0f }},
			{ { 0.0f, 0.0f },{ 0.0f, 0.0f, 0.0f },{ 0.0f, 0.0f, 0.0f }},
			{ { 1.0f, 0.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f, 0.0f }}
			};

			// prepare indices
			std::vector<uint32_t> indices;
			for (uint32_t i = 0; i < 3; ++i)
			{
				uint32_t values[6] = { 0,1,2, 2,3,0 };
				for (auto index : values)
				{
					indices.push_back(i * 4 + index);
				}
			}

			// vertex data
			VulkanAPI::BufferUpdateEvent vertexEvent{ "PlaneModelVertices", vertices.data(), vertices.size() * sizeof(Vertex), VulkanAPI::MemoryUsage::VK_BUFFER_STATIC };
			Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(vertexEvent);

			// index data
			VulkanAPI::BufferUpdateEvent indexEvent{ "PlaneModelIndices", indices.data(), indices.size() * sizeof(uint32_t), VulkanAPI::MemoryUsage::VK_BUFFER_STATIC };
			Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(indexEvent);
		}
	}
}