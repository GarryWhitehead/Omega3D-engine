#include "LightManager.h"

#include "OEMaths/OEMaths_transform.h"

namespace OmegaEngine
{

LightManager::LightManager()
{
}

LightManager::~LightManager()
{
}

void LightManager::calculatePointIntensity(float intensity, PointLight& light)
{
	light.intensity = intensity * static_cast<float>(M_1_PI) * 0.25f;
}

void LightManager::calculateSpotIntensity(float intensity, float outerCone, float innerCone, SpotLight& spotLight)
{
	// first calculate the spotlight cone values
	float outer = std::min(std::abs(outerCone), static_cast<float>(M_PI));
	float inner = std::min(std::abs(innerCone), static_cast<float>(M_PI));
	inner = std::min(inner, outer);

	float cosOuter = std::cos(outer);
	float cosInner = std::cos(inner);
	spotLight.scale = 1.0f / std::max(1.0f / 1024.0f, cosInner - cosOuter);
	spotLight.offset = -cosOuter * spotLight.scale;

	// this is a more focused spot - a unfocused spot would be:#
	// intensity * static_cast<float>(M_1_PI)
	cosOuter = -spotLight.offset / spotLight.scale;
	float cosHalfOuter = std::sqrt((1.0f + cosOuter) * 0.5f);
	spotLight.intensity = intensity / (2.0f * static_cast<float>(M_PI) * (1.0f - cosHalfOuter));
}

void LightManager::addLight(LightBase* light)
{
	switch (light->type)
	{
	case LightType::Spot:
	{
		SpotLight* sLight = static_cast<SpotLight*>(light);
		// carry out some of the calculations on the cpu side to save time
		calculateSpotIntensity(sLight->intensity, sLight->outerCone, sLight->innerCone, *sLight);
        break;
	}
	case LightType::Point:
	{
		PointLight* pLight = static_cast<PointLight*>(light);
		calculatePointIntensity(pLight->intensity, *pLight);
        break;
	}
    case LightType::Directional:
        break;
	}

	// nothing to be done with directional lights right now
	lights.emplace_back(light);
}

size_t LightManager::getLightCount() const
{
	return lights.size();
}

LightBase* LightManager::getLight(const size_t idx)
{
	assert(idx < lights.size());
	return lights[idx];
}

}    // namespace OmegaEngine
