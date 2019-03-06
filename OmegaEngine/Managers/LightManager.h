#pragma once

#include "Managers/ManagerBase.h"

#include "tiny_gltf.h"

#include <cstdint>

namespace OmegaEngine
{

	class LightManager : public ManagerBase
	{

	public:

		LightManager();
		~LightManager();

		// not used at present - just here to keep the inheritance demons happy
		void update_frame(double time, double dt,
			std::unique_ptr<ObjectManager>& obj_manager,
			std::unique_ptr<ComponentInterface>& component_manager) override {}

		void parseGltfLight(uint32_t spaceId, tinygltf::Model& model);

	private:

	};

}

