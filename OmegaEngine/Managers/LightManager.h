#pragma once

#include "Managers/ManagerBase.h"
#include "OEMaths/OEMaths.h"

#include "tiny_gltf.h"

#include <cstdint>
#include <vector>

#define MAX_LIGHTS 100

namespace OmegaEngine
{

	enum class LightType
	{
		Spot,
		Cone,
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
			LightInfo lights[MAX_LIGHTS];
			uint32_t lightCount;
		};

		LightManager();
		~LightManager();

		// not used at present - just here to keep the inheritance demons happy
		void updateFrame(double time, double dt,
			std::unique_ptr<ObjectManager>& objectManager,
			ComponentInterface* componentInterface) override;

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
		LightUboBuffer lightBuffer;

		bool isDirty = false;
	};

}

