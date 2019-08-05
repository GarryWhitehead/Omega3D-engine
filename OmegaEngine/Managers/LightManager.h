#pragma once

#include "Managers/ManagerBase.h"
#include "OEMaths/OEMaths.h"

#include <cstdint>
#include <tuple>
#include <vector>

#define MAX_SPOT_LIGHTS 50
#define MAX_POINT_LIGHTS 50
#define MAX_DIR_LIGHTS 5

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

struct DirectionalLight : public LightBase
{
	float intensity = 10000.0f;
};

struct PointLight : public LightBase
{
	float fallOut = 10.0f;
	float radius = 25.0f;
	float intensity = 1000.0f;
};

struct SpotLight : public LightBase
{
	float fallOut = 10.0f;
	float radius = 25.0f;
	float scale = 1.0f;
	float offset = 0.0f;
	float intensity = 1000.0f;
};

class LightManager : public ManagerBase
{

public:
	// a mirror of the shader structs
	struct PointLightUbo
	{
		PointLightUbo() = default;
		PointLightUbo(const OEMaths::mat4f& mvp, const OEMaths::vec4f& pos, const OEMaths::vec3f& col, float fo,
		              float intensity)
		    : lightMvp(mvp)
		    , position(pos)
		    , colour(OEMaths::vec4f{ col, intensity })
		    , fallOut(fo)

		{
		}

		OEMaths::mat4f lightMvp;
		OEMaths::vec4f position;
		OEMaths::vec4f colour = OEMaths::vec4f{ 1.0f, 1.0f, 1.0f, 1000.0f };
		float fallOut;
	};

	struct SpotLightUbo
	{
		SpotLightUbo() = default;
		SpotLightUbo(const OEMaths::mat4f& mvp, const OEMaths::vec4f& pos, const OEMaths::vec4f& dir,
		             const OEMaths::vec3f& col, float fo, float intensity, float sca, float ofs)
		    : lightMvp(mvp)
		    , position(pos)
		    , direction(dir)
		    , colour(OEMaths::vec4f{ col, intensity })
		    , fallOut(fo)
		    , scale(sca)
		    , offset(ofs)

		{
		}

		OEMaths::mat4f lightMvp;
		OEMaths::vec4f position;
		OEMaths::vec4f direction;
		OEMaths::vec4f colour = OEMaths::vec4f{ 1.0f, 1.0f, 1.0f, 1000.0f };
		float scale;
		float offset;
		float fallOut;
	};

	struct DirectionalLightUbo
	{
		DirectionalLightUbo() = default;
		DirectionalLightUbo(const OEMaths::mat4f& mvp, const OEMaths::vec4f& pos, const OEMaths::vec4f& dir,
		             const OEMaths::vec3f& col, float intensity)
		    : lightMvp(mvp)
		    , position(pos)
		    , direction(dir)
		    , colour(OEMaths::vec4f{ col, intensity })
		{
		}

		OEMaths::mat4f lightMvp;
		OEMaths::vec4f position;
		OEMaths::vec4f direction;
		OEMaths::vec4f colour = OEMaths::vec4f{ 1.0f, 1.0f, 1.0f, 1000.0f };
	};

	struct LightUboBuffer
	{
		SpotLightUbo spotLights[MAX_SPOT_LIGHTS];
		PointLightUbo pointLights[MAX_POINT_LIGHTS];
		DirectionalLightUbo dirLights[MAX_DIR_LIGHTS];			// not anticipating as many directional lights required
	};

	LightManager();
	~LightManager();

	// not used at present - just here to keep the inheritance demons happy
	void updateFrame(double time, double dt, std::unique_ptr<ObjectManager>& objectManager,
	                 ComponentInterface* componentInterface) override;

	void updateLightPositions(double time, double dt);

	void updateDynamicBuffer(ComponentInterface* componentInterface);

	void calculatePointIntensity(float intensity, PointLight& light);
	void calculateSpotIntensity(float intensity, float outerCone, float innerCone, SpotLight& spotLight);

	void addSpotLight(const OEMaths::vec3f& position, const OEMaths::vec3f& target, const OEMaths::vec3f& colour,
	                  const float fov, float intensity, float fallout, float innerCone, float outerCone,
	                  const LightAnimateType animType = LightAnimateType::Static, const float animVel = 0.0f);

	void addPointLight(const OEMaths::vec3f& position, const OEMaths::vec3f& target, const OEMaths::vec3f& colour,
	                   float fov, float intensity, float fallOut,
	                   const LightAnimateType animType = LightAnimateType::Static, const float animVel = 0.0f);

	void LightManager::addDirectionalLight(const OEMaths::vec3f& position, const OEMaths::vec3f& target,
	                                       const OEMaths::vec3f& colour, float fov, float intensity);

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
