#pragma once

#include "Types/MappedTexture.h"

#include "Utility/String.h"

#include <cstdint>
#include <utility>

namespace OmegaEngine
{

/**
* @brief Used to add textures to the vulkan back-end were they will be hosted on the GPU.
* This is for grouped textures were many textures are linked under the same id. This is
* useful for materials for instnace which have many elements described in the same descriptor set
*/
class GpuTextureInfo
{
public:
	GpuTextureInfo(Util::String id, uint32_t binding, MappedTexture* mapped)
	    : id(id)
	    , binding(binding)
	    , texture(mapped)
	{
	}

	// no copy
	GpuTextureInfo(GpuTextureInfo&) = delete;
	GpuTextureInfo& operator=(GpuTextureInfo&) = delete;
    
    friend class RenderableManager;
    
private:
    
	Util::String id;
	uint32_t binding = 0;
	MappedTexture* texture = nullptr;
};

/**
	* @brief Used to define a buffer object. Will be passed to the buffer manager
	* on creation of the renderer on the vulkan backend, and used to prepare the
	* gpu memory space for the buffer. Buffers and their relevant descriptor sets are linked 
	* via the id. The buffer name must refer to the uniform buffer - for example:
	* "CameraUbo" - the buffer must be created with the id "Camera".
	*/
class GpuBufferInfo
{
public:
	GpuBufferInfo(const char* id, void* data, size_t size, MemoryUsage _usage)
	    : id(id)
	    , data(data)
	    , size(size)
	    , memoryType(usage)
	{
	}

	// no copy or assignmet
	GpuBufferInfo(const GpuBufferInfo&) = delete;
	GpuBufferInfo& operator=(const GpuBufferInfo&) = delete;
    
private:
	// buffer id - see brief above regards naming convention
	Util::String id;

	// pointer to the data which will be hosted on the gpu
	void* data = nullptr;

	// size of the data
	size_t size = 0;

	// handle to where this is located in the vulkan backend
    BufferHandle handle;
};

}    // namespace OmegaEngine
