#pragma once

#include "tiny_gltf.h"

namespace OmegaEngine
{

	class LightManager
	{

	public:

		LightManager();
		~LightManager();

		void parseGltfLight(uint32_t spaceId, tinygltf::Model& model);

	private:

	};

}

