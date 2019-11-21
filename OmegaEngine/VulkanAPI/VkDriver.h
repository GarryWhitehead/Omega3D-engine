#pragma once

#include "VulkanAPI/Buffer.h"
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
class VertexBuffer;
class IndexBuffer;
class Swapchain;

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

/**
 * @brief Resources are hashed using the data pointer as this will be unique for each resource
 */
namespace VkHash
{

void* ptr;

struct ResourceHasher
{
	size_t operator()(void* ptr) const noexcept
	{
		return std::hash<void*>{}(ptr);
	}
};

struct ResourceEqualTo
{
	bool operator()(void* lhs, const void* rhs) const
	{
		return lhs == rhs;
	}
};

using TextureMap = std::unordered_map<void*, Texture, ResourceHasher, ResourceEqualTo>;
using BufferMap = std::unordered_map<void*, Buffer, ResourceHasher, ResourceEqualTo>;
using VertexMap = std::unordered_map<void*, VertexBuffer, ResourceHasher, ResourceEqualTo>;
using IndexMap = std::unordered_map<void*, IndexBuffer, ResourceHasher, ResourceEqualTo>;

};    // namespace VkHash

using DynBufferHandle = uint64_t;

class VkDriver
{

public:
	VkDriver();
	~VkDriver();

	/// initialises the vulkan driver - includes creating the abstract device, physical device, queues, etc.
	void init(const char** instanceExt, uint32_t count);

	/// Make sure you call this before closing down the engine!
	void shutdown();

	// Functions for creating buffers and adding resource data to the backend
	/**
     * @brief This is for adding dynamic buffers which will be destroyed each frame - such as camera ubos, etc.
     * @param size The size of the buffer in bytes
     * @param usage Vulkan usage flags depict what this buffer will be used for
     * @param data (optional) A pointer to the buffer data which will be mapped to the newly created buffer
     */
	DynBufferHandle addDynamicUbo(const size_t size, VkBufferUsageFlags usage, void* data = nullptr);

	/**
     * @brief This is for adding persistant uniform buffers to the backend. These will remain in the backend until the user calls the appropiate destroy function.
     * @param size The size of the buffer in bytes
     * @param usage Vulkan usage flags depict what this buffer will be used for
     * @param data (optional) A pointer to the buffer data which will be mapped to the newly created buffer
     */
	void addStaticUbo(const size_t size, VkBufferUsageFlags usage, void* data = nullptr);

	/**
     * @brief Adds a vertex buffer to the vulkan back end. If the vertex buffer is already present in the map, then
     * this won't be added - note: there is no indication of this - the reason being that no dirty flag state is used to state
     * whether the textures have changed. This function also generates the vertex attribute bindings in preperation to using with the relevant pipeline
     */
	void addVertexBuffer(const size_t size, void* data, std::vector<VertexBuffer::Attribute>& attributes);

	/**
     @brief Similiar to the **addVertexBuffer** function, adds a index buffer to the vulkan backend. Note: it is presumed to be of the type uint32_t.
    */
	void addIndexBuffer(const size_t size, uint32_t* data);

	void add2DTexture(const vk::Format format, const uint32_t width, const uint32_t height, const uint8_t mipLevels,
	                  void* data);

	// ======== begin/end frame functions ====================
	void beginFrame();

	void endFrame();

	// ====== manager helper functions =========
	CommandBufferManager& getCmdBufManager()
	{
		return *cbManager;
	}

	ProgramManager& getProgManager()
	{
		return *progManager;
	}

	VkContext& getContext()
	{
		return context;
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
	VkHash::TextureMap textures;
	VkHash::BufferMap buffers;
	VkHash::VertexMap vertBuffers;
	VkHash::IndexMap indexBuffers;

	// Keep local track of the swapchain
	// TODO: This may need chnaging if multiple swapchains are used!
	Swapchain& swapchain;

#ifdef VULKAN_VALIDATION_DEBUG

	vk::DebugReportCallbackEXT debugCallback;
	vk::DebugUtilsMessengerEXT debugMessenger;

#endif
};

}    // namespace VulkanAPI
