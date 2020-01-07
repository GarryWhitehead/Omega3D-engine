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
