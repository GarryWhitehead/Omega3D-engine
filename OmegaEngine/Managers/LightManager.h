#pragma once

#include "Managers/ManagerBase.h"
#include "OEMaths/OEMaths.h"

#include <cstdint>
#include <tuple>
#include <vector>

#define MAX_LIGHTS 50

namespace OmegaEngine
{

struct LightPOV
{
	OEMaths::mat4f lightMvp;
};

enum class LightType
{
	Spot,
	Point,
	Directional
};

enum class LightAnimateType
{
	Static,
	RotateX,
	RotateY,
	RotateZ
};

struct LightAnimateInfo
{
	LightAnimateInfo() = default;

	LightAnimateInfo(const LightAnimateType type, const float vel)
	    : animationType(type)
	    , velocity(vel)
	{
	}

	// default to static
	LightAnimateType animationType = LightAnimateType::Static;
	float velocity = 5.0f;
};

struct LightBase
{
	LightBase() = default;
	virtual ~LightBase()
	{
	}

	OEMaths::mat4f lightMvp;
	OEMaths::vec3f position;
	OEMaths::vec3f target;
	OEMaths::vec3f colour = OEMaths::vec3f{ 1.0f, 1.0f, 1.0f };
	float fov = 90.0f;
	LightType type;
};

struct PointLight : public LightBase
{
	float radius = 25.0f;
};

struct SpotLight : public LightBase
{
	OEMaths::vec3f direction;
	float radius = 25.0f;
	float scale = 1.0f;
	float offset = 0.0f;
};

class LightManager : public ManagerBase
{

public:
	// a mirror of the shader structs
	struct PointLightUbo
	{
		PointLightUbo() = default;
		PointLightUbo(const OEMaths::mat4f& mvp, const OEMaths::vec4f& pos, const OEMaths::vec3f& col, float rad)
		    : lightMvp(mvp)
		    , position(pos)
		    , colour(col)
		    , radius(rad)

		{
		}

		OEMaths::mat4f lightMvp;
		OEMaths::vec4f position;
		OEMaths::vec3f colour = OEMaths::vec3f{ 1.0f, 1.0f, 1.0f };
		float radius;
	};

	struct SpotLightUbo
	{
		SpotLightUbo() = default;
		SpotLightUbo(const OEMaths::mat4f& mvp, const OEMaths::vec4f& pos, const OEMaths::vec4f& dir,
		             const OEMaths::vec3f& col, float rad, float sca, float ofs)
		    : lightMvp(mvp)
		    , position(pos)
		    , direction(dir)
		    , colour(col)
		    , radius(rad)
		    , scale(sca)
		    , offset(ofs)
		{
		}

		OEMaths::mat4f lightMvp;
		OEMaths::vec4f position;
		OEMaths::vec4f direction;
		OEMaths::vec3f colour = OEMaths::vec3f{ 1.0f, 1.0f, 1.0f };
		float pad0;
		float radius;
		float scale;
		float offset;
	};

	struct LightUboBuffer
	{
		SpotLightUbo spotLights[MAX_LIGHTS];
		PointLightUbo pointLights[MAX_LIGHTS];
		float pad[2];
		uint32_t spotLightCount = 0;
		uint32_t pointLightCount = 0;
	};

	LightManager();
	~LightManager();

	// not used at present - just here to keep the inheritance demons happy
	void updateFrame(double time, double dt, std::unique_ptr<ObjectManager>& objectManager,
	                 ComponentInterface* componentInterface) override;

	void updateLightPositions(double time, double dt);

	void updateDynamicBuffer(ComponentInterface* componentInterface);

	void addSpotLight(const OEMaths::vec3f& position, const OEMaths::vec3f& target, const OEMaths::vec3f& colour,
	                  const float fov, const OEMaths::vec3f dir, float radius, float scale, float offset,
	                  const LightAnimateType animType = LightAnimateType::Static, const float animVel = 0.0f);

	void addPointLight(const OEMaths::vec3f& position, const OEMaths::vec3f& target, const OEMaths::vec3f& colour,
	                   float fov, float radius, const LightAnimateType animType = LightAnimateType::Static,
	                   const float animVel = 0.0f);

	uint32_t getLightCount() const
	{
		return static_cast<uint32_t>(lights.size());
	}

	uint32_t getAlignmentSize() const
	{
		return alignedPovDataSize;
	}

private:
	std::vector<std::tuple<std::unique_ptr<LightBase>, LightAnimateInfo>> lights;

	// buffer on the vulkan side which will hold all lighting info
	LightUboBuffer lightBuffer;

	// dynamic buffer for light pov - used for shadow drawing
	LightPOV* lightPovData = nullptr;
	uint32_t alignedPovDataSize = 0;
	uint32_t lightPovDataSize = 0;

	// dirty timer for light animations
	float timer = 0.0f;

	bool isDirty = false;
};

}    // namespace OmegaEngine
