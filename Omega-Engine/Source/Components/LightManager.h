#pragma once

#include "OEMaths/OEMaths.h"

#include "Components/ComponentManager.h"

#include "omega-engine/LightManager.h"

#include <cstdint>
#include <vector>
#include <memory>

namespace OmegaEngine
{

// forward declerations
class OEWorld;
class Shadow;
class OELightManager;

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
	friend class OEScene;
	friend class OELightManager;
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
    
	friend class OEScene;
	friend class OELightManager;
    friend class LightManager;

private:

};

struct PointLight : public LightBase
{
public:
    
    PointLight() :
        LightBase(LightType::Point)
    {}
    
	friend class OEScene;
	friend class OELightManager;
    friend class LightManager;

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
    
	friend class OEScene;
	friend class OELightManager;
    friend class LightManager;
    
private:

	float fallout;
	float radius;
	float scale;
	float offset;

    // this needs looking at - not set at present
    float innerCone = 0.0f;
    float outerCone = 0.0f;
};

class OELightManager : public ComponentManager, public LightManager
{

public:
    
	OELightManager();
	~OELightManager();

	void calculatePointIntensity(float intensity, PointLight& light);
	void calculateSpotIntensity(float intensity, float outerCone, float innerCone, SpotLight& spotLight);

    void addLight(std::unique_ptr<LightBase>& light, OEObject* obj);

	size_t getLightCount() const;
    LightBase* getLight(const ObjectHandle& handle);
    
    friend class Shadow;
    
private:

	std::vector<std::unique_ptr<LightBase>> lights;

};

}    // namespace OmegaEngine
