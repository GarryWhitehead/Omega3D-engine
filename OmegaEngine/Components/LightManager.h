#pragma once

#include "OEMaths/OEMaths.h"

#include "Components/ComponentManager.h"

#include <cstdint>
#include <vector>

#define MAX_SPOT_LIGHTS 50
#define MAX_POINT_LIGHTS 50
#define MAX_DIR_LIGHTS 5

namespace OmegaEngine
{

// forward declerations
class World;

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

class LightBase
{

public:

	LightBase(LightType type)
	    : type(type)
	{
	}

	virtual ~LightBase() = default;

	LightBase() = delete;

	friend class Scene;
	friend class LightManager;

protected:

	/// the projection matrix of the light taken from the lights point-of-view
	/// this is for shadow drawing
	OEMaths::mat4f lightMvp;
	
	// position of the light in world space 
	OEMaths::vec3f position;
	OEMaths::vec3f target;

	/// the colour of the light
	OEMaths::vec3f colour = OEMaths::vec3f{ 1.0f };

	/// the field of view of this light
	float fov = 90.0f;

	/// Whether this is directional, spot or point light
	LightType type;

	/// states whether this light is visible. Set by the visibility check during scene update
	bool isVisible = false;
};

struct DirectionalLight : public LightBase
{
public:

	friend class Scene;
	friend class LightManager;

private:

	/// the light intensity in lumens
	float intensity = 10000.0f;
};

struct PointLight : public LightBase
{
public:

	friend class Scene;
	friend class LightManager;

private:

	float fallOut = 10.0f;
	
	/// the radius of the light in pixels
	float radius = 25.0f;
	
	/// the light intensity in lumens
	float intensity = 1000.0f;
};

struct SpotLight : public LightBase
{
public:

	friend class Scene;
	friend class LightManager;

private:

	float fallout = 10.0f;
	float radius = 25.0f;
	float scale = 1.0f;
	float offset = 0.0f;

    // this needs looking at - not set at present
    float innerCone = 0.0f;
    float outerCone = 0.0f;
    
	/// the light intensity in lumens
	float intensity = 1000.0f;
};

class LightManager : public ComponentManager
{

public:
	
	LightManager();
	~LightManager();


	void calculatePointIntensity(float intensity, PointLight& light);
	void calculateSpotIntensity(float intensity, float outerCone, float innerCone, SpotLight& spotLight);

	void addLight(LightBase* light);

	size_t getLightCount() const;
	LightBase* getLight(const size_t idx);

private:

	std::vector<LightBase*> lights;



	// dynamic buffer for light pov - used for shadow drawing
	LightPOV* lightPovData = nullptr;
	uint32_t alignedPovDataSize = 0;
	uint32_t lightPovDataSize = 0;
};

}    // namespace OmegaEngine
