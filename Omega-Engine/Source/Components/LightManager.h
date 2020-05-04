/* Copyright (c) 2018-2020 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "Components/ComponentManager.h"
#include "OEMaths/OEMaths.h"
#include "omega-engine/LightManager.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace OmegaEngine
{

// forward declerations
class OEWorld;
class Shadow;
class OELightManager;
class OECamera;

class LightBase
{

public:
    LightBase(LightType type) : type(type)
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
    OEMaths::vec3f colour = OEMaths::vec3f {1.0f};

    /// the field of view of this light
    float fov = 90.0f;

    /// the light intensity in lumens
    float intensity = 100.0f;

    /// Whether this is directional, spot or point light
    LightType type;

    /// states whether this light is visible. Set by the visibility check during scene update
    bool isVisible = false;
};

struct DirectionalLight : public LightBase
{
public:
    DirectionalLight() : LightBase(LightType::Directional)
    {
    }

    friend class OEScene;
    friend class OELightManager;
    friend class LightManager;

private:
};

struct PointLight : public LightBase
{
public:
    PointLight() : LightBase(LightType::Point)
    {
    }

    friend class OEScene;
    friend class OELightManager;
    friend class LightManager;

private:
    
    float fallOut = 0.0f;
    float radius = 0.0f;
};

struct SpotLight : public LightBase
{
public:
    SpotLight() : LightBase(LightType::Spot)
    {
    }

    friend class OEScene;
    friend class OELightManager;
    friend class LightManager;

private:
    
    float fallout = 0.0f;
    float radius = 0.0f;
    float scale = 0.0f;
    float offset = 0.0f;

    // used for deriving the spotlight intensity
    float innerCone = 5.0f;
    float outerCone = 10.0f;
};

class OELightManager : public ComponentManager, public LightManager
{

public:
    OELightManager();
    ~OELightManager();
    
    void update(const OECamera& camera);
    
    void calculatePointIntensity(float intensity, PointLight& light);
    void
    calculateSpotIntensity(float intensity, float outerCone, float innerCone, SpotLight& spotLight);

    void addLight(std::unique_ptr<LightBase>& light, OEObject* obj);

    size_t getLightCount() const;
    LightBase* getLight(const ObjectHandle& handle);

    friend class Shadow;

private:
    
    std::vector<std::unique_ptr<LightBase>> lights;
    
    bool isDirty = false;
};

} // namespace OmegaEngine
