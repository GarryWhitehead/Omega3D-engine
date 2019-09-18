#pragma once

#include "utility/String.h"

#include <utility>

namespace OmegaEngine
{

/**
	* @brief Used to define a buffer object. Will be passed to the buffer manager
	* on creation of the renderer on the vulkan backend, and used to prepare the
	* gpu memory space for the buffer. Buffers and their relevant descriptor sets are linked 
	* via the id. The buffer name must refer to the uniform buffer - for example:
	* "CameraUbo" - the buffer must be created with the id "Camera".
	*/
class UBufferCreateInfo
{
public:
	UBufferCreateInfo(const char* _id, void* _data, size_t _size, MemoryUsage _usage)
	    : id(_id)
	    , data(_data)
	    , size(_size)
	    , memoryType(_usage)
	{
	}

	// no copy or assignmet
	UBufferCreateInfo(const UBufferCreateInfo&) = delete;
	UBufferCreateInfo& operator=(const UBufferCreateInfo&) = delete;

	// move and assignment allowed though
	UBufferCreateInfo(UBufferCreateInfo&& rhs)
	    : id(rhs.id)
	    , data(std::exchange(rhs.data, nullptr))
	    , size(std::exchange(rhs.size, 0))
	    , memoryType(rhs.memoryType)
	{
	}

	UBufferCreateInfo& operator=(UBufferCreateInfo&& rhs)
	{
		if (this != &rhs)
		{
			id = rhs.id;
			std::exchange(rhs.data, nullptr);
			std::exchange(rhs.size, 0);
			memoryType = rhs.memoryType;
		}
		return *this;
	}

	friend class VulkanAPI::BufferManager;

private:
	// buffer id - see brief above regards naming convention
	Util::String id;

	// pointer to the data which will be hosted on the gpu
	void* data = nullptr;

	// size of the data
	size_t size = 0;

	// Determines the type of memory that this buffer will be stored in
	// Static memory is hosted on the GPU dedicated mem, and should be used
	// for buffers which will not chnange. Alternatively, dynamic memory is
	// CPU-visible memory and should be used for buffers were data may change
	// often.
	MemoryUsage memoryType;
};

/**
	* @ brief Similiar to the **UBufferCreateInfo** class. This class is used
	* to update data in a buffer that has already been created on the GPU host
	*/
class UBufferUpdateInfo
{
public:
	UBufferUpdateInfo(const char* _id, void* _data, size_t _size, bool _flush)
	    : id(_id)
	    , data(_data)
	    , size(_size)
	    , flushMem(_flush)
	{
	}

	// no copy or assignmet
	UBufferUpdateInfo(const UBufferUpdateInfo&) = delete;
	UBufferUpdateInfo& operator=(const UBufferUpdateInfo&) = delete;

	// move and assignment allowed though
	UBufferUpdateInfo(UBufferUpdateInfo&& rhs)
	    : id(rhs.id)
	    , data(std::exchange(rhs.data, nullptr))
	    , size(std::exchange(rhs.size, 0))
	    , flushMem(rhs.flushMem)
	{
	}

	UBufferUpdateInfo& operator=(UBufferUpdateInfo&& rhs)
	{
		if (this != &rhs)
		{
			id = rhs.id;
			std::exchange(rhs.data, nullptr);
			std::exchange(rhs.size, 0);
			flushMem = rhs.flushMem;
		}
		return *this;
	}

	friend class VulkanAPI::BufferManager;

private:
	// buffer id - see brief above regards naming convention
	Util::String id;

	// pointer to the data which will be hosted on the gpu
	void* data = nullptr;

	// size of the data
	size_t size = 0;

	// ensures that non-chorent memory is flushed from host caches
	// note - should not be used with coherent memory as there may
	// be a perfomance penalty
	bool flushMem = false;
};

}    // namespace OmegaEngine
