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

#ifndef LIGHTMANAGER_HPP
#define LIGHTMANAGER_HPP

#include "utility/Compiler.h"

#include "OEMaths/OEMaths.h"

namespace OmegaEngine
{
class Engine;
class Object;

enum class LightType
{
    Spot,
    Point,
    Directional,
    None
};

class OE_PUBLIC LightManager
{

public:
    
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
        
        void create(Engine& engine, Object* obj);
        
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
  
    LightManager() = default;
    
private:
    
};

}

#endif /* LIGHTMANAGER_HPP */
