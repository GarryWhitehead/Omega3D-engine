#pragma once

#include "VulkanAPI/Common.h"

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

	void addBuffer(size_t size, VkBufferUsageFlags usage);

	void addVertexBuffer(size_t size, uint8_t attribCount);

	void addIndexBuffer(size_t size);

	void addImage2D(uint32_t width, uint32_t height, uint8_t mipLevels, void* data);

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

	// resources associated with this device
	std::unordered_map<VkHandle, Texture> textures;
	std::unordered_map<VkHandle, Buffer> buffers;

#ifdef VULKAN_VALIDATION_DEBUG

	vk::DebugReportCallbackEXT debugCallback;
	vk::DebugUtilsMessengerEXT debugMessenger;

#endif

};

}    // namespace VulkanAPI
