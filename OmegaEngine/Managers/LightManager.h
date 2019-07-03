#pragma once

#include "Managers/ManagerBase.h"
#include "OEMaths/OEMaths.h"

#include "tiny_gltf.h"

#include <cstdint>
#include <vector>

#define MAX_LIGHTS 100

namespace OmegaEngine
{

struct LightPOV
{
	OEMaths::mat4f lightMvp;
};

enum class LightType
{
	Spot,
	Cone,
	Directional
};

struct LightInfo
{
	OEMaths::vec3f position;
	float pad0;
	OEMaths::vec3f target;
	float pad1;
	OEMaths::vec3f colour = OEMaths::vec3f{ 1.0f, 1.0f, 1.0f };
	float fov = 90.0f;
	float radius = 25.0f;
	float innerCone = 0.0f; // for spot lights
	float outerCone = 0.0f; // for spot lights
	LightType type = LightType::Spot;
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
	void updateFrame(double time, double dt, std::unique_ptr<ObjectManager> &objectManager,
	                 ComponentInterface *componentInterface) override;

	void updateDynamicBuffer(ComponentInterface *componentInterface);

	void parseGltfLight(uint32_t spaceId, tinygltf::Model &model);

	void addLight(const LightType type, OEMaths::vec3f &position, OEMaths::vec3f &target,
	              OEMaths::vec3f &colour, float radius, float fov, float innerCone,
	              float outerCone);

	void addLight(LightInfo &light);

	uint32_t getLightCount() const
	{
		return static_cast<uint32_t>(lights.size());
	}

	uint32_t getAlignmentSize() const
	{
		return alignedPovDataSize;
	}

private:
	std::vector<LightInfo> lights;

	// buffer on the vulkan side which will hold all lighting info
	LightUboBuffer lightBuffer;

	// dynamic buffer for light pov - used for shadow drawing
	LightPOV *lightPovData = nullptr;
	uint32_t alignedPovDataSize = 0;
	uint32_t lightPovDataSize = 0;

	bool isDirty = false;
};

} // namespace OmegaEngine
