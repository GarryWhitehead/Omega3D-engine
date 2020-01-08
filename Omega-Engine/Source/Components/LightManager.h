#pragma once

#include "OEMaths/OEMaths.h"

#include "Components/ComponentManager.h"

#include <cstdint>
#include <vector>
#include <memory>

namespace OmegaEngine
{

// forward declerations
class World;
class Shadow;
class Engine;
class LightManager;

enum class LightType
{
	Spot,
	Point,
	Directional,
    None
};

class LightInstance
{
public:
    
    LightInstance& setType(LightType lt);
    LightInstance& setPosition(const OEMaths::vec3f p);
    LightInstance& setColour(const OEMaths::colour3 c);
    LightInstance& setFov(float f);
    LightInstance& setIntensity(float i);
    LightInstance& setFallout(float fo);
    LightInstance& setRadius(float r);
    
    void create(Engine& engine);
    
    friend class LightManager;
    
private:
    
    LightType type = LightType::None;
    OEMaths::vec3f pos;
    OEMaths::colour3 col = OEMaths::colour3{1.0f};
    float fov = 90.0f;
    float intensity = 1000.0f;
    float fallout = 10.0f;
    float radius = 20.0f;
    float scale = 1.0f;
    float offset = 0.0f;
};

class LightBase
{

public:
        
	LightBase(LightType type)
	    : type(type)
	{
	}

	virtual ~LightBase() = default;
    
    // the public setters
    LightBase& setPosition(const OEMaths::vec3f pos)
    {
        position = pos;
        return *this;
    }
    
    LightBase& setColour(const OEMaths::vec3f col)
    {
        colour = col;
        return *this;
    }
    
    friend class Shadow;
	friend class Scene;
	friend class LightManager;
    friend class LightInstance;

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

    /// the light intensity in lumens
    float intensity;
    
	/// Whether this is directional, spot or point light
	LightType type;

	/// states whether this light is visible. Set by the visibility check during scene update
	bool isVisible = false;
};

struct DirectionalLight : public LightBase
{
public:
    
    DirectionalLight() :
        LightBase(LightType::Directional)
    {}
    
	friend class Scene;
	friend class LightManager;
    friend class LightInstance;

private:

};

struct PointLight : public LightBase
{
public:
    
    PointLight() :
        LightBase(LightType::Point)
    {}
    
	friend class Scene;
	friend class LightManager;
    friend class LightInstance;

private:

	float fallOut;
	float radius;
};

struct SpotLight : public LightBase
{
public:
    
    SpotLight() :
        LightBase(LightType::Spot)
    {}
    
	friend class Scene;
	friend class LightManager;
    friend class LightInstance;
    
private:

	float fallout;
	float radius;
	float scale;
	float offset;

    // this needs looking at - not set at present
    float innerCone = 0.0f;
    float outerCone = 0.0f;
};

class LightManager : public ComponentManager
{

public:
	
    static constexpr uint32_t MAX_SPOT_LIGHTS = 50;
    static constexpr uint32_t MAX_POINT_LIGHTS = 50;
    static constexpr uint32_t MAX_DIR_LIGHTS = 5;
    
	LightManager();
	~LightManager();

	void calculatePointIntensity(float intensity, PointLight& light);
	void calculateSpotIntensity(float intensity, float outerCone, float innerCone, SpotLight& spotLight);

    void addLight(std::unique_ptr<LightBase>& light);

	size_t getLightCount() const;
	LightBase* getLight(const size_t idx);
    
    friend class Shadow;
    
private:

	std::vector<std::unique_ptr<LightBase>> lights;

};

}    // namespace OmegaEngine
