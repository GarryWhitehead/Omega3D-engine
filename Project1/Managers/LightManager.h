#pragma once

#include "ComponentInterface/ComponentManagerBase.h"
#include "tiny_gltf.h"

namespace OmegaEngine
{

	class LightManager : public ComponentManagerBase
	{

	public:

		LightManager();
		~LightManager();

		void parseGltfLight(uint32_t spaceId, tinygltf::Model& model);

	private:

	};

}

