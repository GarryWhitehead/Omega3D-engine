#pragma once

#include "VulkanAPI/Common.h"
#include "VulkanAPI/Buffer.h"
#include "VulkanAPI/VkContext.h"

#include <unordered_map>
#include <vector>

namespace VulkanAPI
{

// forward declerations
class ProgramManager;
class CommandBufferManager;
class Buffer;
class Texture;

/**
* @brief A wrapper for a vulkan instance
*/
class Instance
{
public:
	// no copying allowed!
	Instance(const Instance&) = delete;
	Instance& operator=(const Instance&) = delete;

private:

	vk::Instance instance;
};

using VkBufferHandle = uint64_t;
using VkVertexHandle = uint64_t;
using VkIndexHandle = uint64_t;
using VkTex2dHandle = uint64_t;

class VkDriver
{

public:

	VkDriver();
	~VkDriver();

	void init();
	void shutdown();

	VkContext& getContext()
	{
		return context;
	}

	VkBufferHandle addBuffer(const size_t size, VkBufferUsageFlags usage) const;

	VkVertexHandle addVertexBuffer(const size_t size, const uint8_t attribCount, void *data) const;

	VkIndexHandle addIndexBuffer(const size_t size, void* data) const ;

	VkTex2dHandle add2DTexture(const vk::Format format, const uint32_t width, const uint32_t height, const uint8_t mipLevels, void* data) const;

	// ====== manager helper functions =========
	CommandBufferManager& getCmdBufManager()
	{
		return *cbManager;
	}

	ProgramManager& getProgManager()
	{
		return *progManager;
	}

private:
	// managers
	std::unique_ptr<ProgramManager> progManager;
	std::unique_ptr<CommandBufferManager> cbManager;

	// the current device context
	VkContext context;

	// external mem allocator
	VmaAllocator vmaAlloc;
    
    // staging pool used for managing CPU stages
    StagingPool stagingPool;
    
	// resources associated with this device
	std::vector<Texture> textures;
	std::vector<Buffer> buffers;
    std::vector<VertexBuffer> vertBuffers;
    std::vector<IndexBuffer> indexBuffers;

#ifdef VULKAN_VALIDATION_DEBUG

	vk::DebugReportCallbackEXT debugCallback;
	vk::DebugUtilsMessengerEXT debugMessenger;

#endif

};

}    // namespace VulkanAPI
