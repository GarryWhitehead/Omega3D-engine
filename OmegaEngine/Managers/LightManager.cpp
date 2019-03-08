#include "LightManager.h"
#include "Objects/ObjectManager.h"
#include "Managers/ComponentInterface.h"

#include "tiny_gltf.h"

namespace OmegaEngine
{

	LightManager::LightManager()
	{
	}


	LightManager::~LightManager()
	{
	}

	void LightManager::update_frame(double time, double dt,
		std::unique_ptr<ObjectManager>& obj_manager,
		ComponentInterface* component_manager)
	{

	}

	void LightManager::parseGltfLight(uint32_t spaceId, tinygltf::Model& model)
	{
		
	}

}
