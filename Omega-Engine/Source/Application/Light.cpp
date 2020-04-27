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

#include "Light.h"

#include <cmath>
#include <algorithm>

void Light::updateLightPositions(double dt)
{
	// update the timer first - a pretty simple fudged timer but adequate for lighting
	// TODO: make this a config option
	constexpr float timerSpeed = 0.1f;

	timer += timerSpeed * (dt / 10000000);

	// clamp to 0.0f - 1.0f
	if (timer > 1.0)
	{
		timer -= 1.0f;
	}
    // TODO!!
	/*for (auto& info : lights)
	{
		auto& light = std::get<0>(info);
		auto& anim = std::get<1>(info);

		switch (anim.animationType)
		{
		case LightAnimateType::Static:
			break;
		case LightAnimateType::RotateX:
		{
			light->position.y = std::abs(std::sin(OEMaths::radians(timer * 360.0f)) * anim.velocity);
			light->position.z = std::cos(OEMaths::radians(timer * 360.0f)) * anim.velocity;
			break;
		}
		case LightAnimateType::RotateY:
		{
			light->position.x = std::abs(std::sin(OEMaths::radians(timer * 360.0f)) * anim.velocity);
			light->position.z = std::cos(OEMaths::radians(timer * 360.0f)) * anim.velocity;
			break;
		}
		case LightAnimateType::RotateZ:
		{
			light->position.x = std::abs(std::sin(OEMaths::radians(timer * 360.0f)) * anim.velocity);
			light->position.y = std::cos(OEMaths::radians(timer * 360.0f)) * anim.velocity;
			break;
		}
		}

		isDirty = true;
	}*/
}
