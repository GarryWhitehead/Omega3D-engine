#pragma once

#include "Managers/ManagerBase.h"
#include "Omega_Common.h"
#include <cstdint>

namespace OmegaEngine
{

	class LightManager : public ManagerBase
	{

	public:

		LightManager();
		~LightManager();

		void parseGltfLight(uint32_t spaceId, tinygltf::Model& model);

	private:

	};

}

