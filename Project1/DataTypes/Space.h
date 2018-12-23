#pragma once
#include "tiny_gltf.h"

// forward declerations
class MappedTexture;

namespace OmegaEngine
{
	struct SpaceInfo
	{

		SpaceInfo(const char* name) : 
			filename(name)
		{}


		const char* filename;
		bool isLoaded = false;
		bool isActive = false;
		uint32_t spaceId;

		uint32_t vertexOffset;
		uint32_t indexOffset;
	};

}

