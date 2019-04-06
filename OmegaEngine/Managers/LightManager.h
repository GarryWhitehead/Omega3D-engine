#pragma once

#include "Managers/ManagerBase.h"
#include "Vulkan/MemoryAllocator.h"
#include "Vulkan/Vulkan_Global.h"
#include "OEMaths/OEMaths.h"

#include "tiny_gltf.h"

#include <cstdint>
#include <vector>

#define MAX_LIGHT_COUNT 100

namespace OmegaEngine
{

	enum class LightType
	{
		Spot,
		Point,
		Directional
	};

	struct LightInfo
	{
		OEMaths::vec4f postion;
		OEMaths::vec3f colour;
		float pad0;
		float radius = 0.0f;
		float innerCone = 0.0f;		// for spot lights
		float outerCone = 0.0f;		// for spot lights
		LightType type;
	};

	class LightManager : public ManagerBase
	{

	public:

		// a mirror of the shader buffer
		struct LightUboBuffer
		{
			LightInfo lights[MAX_LIGHT_COUNT];
			uint32_t lightCount;
		};

		LightManager();
		~LightManager();

		// not used at present - just here to keep the inheritance demons happy
		void update_frame(double time, double dt,
			std::unique_ptr<ObjectManager>& obj_manager,
			ComponentInterface* component_manager) override;

		void parseGltfLight(uint32_t spaceId, tinygltf::Model& model);

		void add_light(LightInfo& light)
		{
			lights.push_back(light);
			// make sure this light is updated on the GPU 
			isDirty = true;		
		}


	private:
	
		std::vector<LightInfo> lights;

		// buffer on the vulkan side which will hold all lighting info 
		LightUboBuffer light_buffer;

		bool isDirty = false;
	};

}

