#include "LightManager.h"

#include "Core/Engine.h"

#include "OEMaths/OEMaths_transform.h"

#include "utility/Logger.h"

namespace OmegaEngine
{

LightManager::LightInstance& LightManager::LightInstance::setType(LightType lt)
{
    type = lt;
    return *this;
}

LightManager::LightInstance& LightManager::LightInstance::setPosition(const OEMaths::vec3f p)
{
    pos = p;
    return *this;
}

LightManager::LightInstance& LightManager::LightInstance::setColour(const OEMaths::colour3 c)
{
    col = c;
    return *this;
}

LightManager::LightInstance& LightManager::LightInstance::setFov(float f)
{
    fov = f;
    return *this;
}

LightManager::LightInstance& LightManager::LightInstance::setIntensity(float i)
{
    intensity = i;
    return *this;
}

LightManager::LightInstance& LightManager::LightInstance::setFallout(float fo)
{
    fallout = fo;
    return *this;
}

LightManager::LightInstance& LightManager::LightInstance::setRadius(float r)
{
    radius = r;
    return *this;
}

void LightManager::LightInstance::create(Engine& engine)
{
    OEEngine& oeEngine = reinterpret_cast<OEEngine&>(engine);
    
    if (type == LightType::None)
    {
        LOGGER_WARN("You haven't specified a light type.");
        return;
    }
    
    auto* lManager = oeEngine.getLightManager();
    
    std::unique_ptr<LightBase> base = std::make_unique<LightBase>(type);
    base->position = pos;
    base->colour = col;
    base->fov = fov;
    base->intensity = intensity;
    
    // create the light based on the type
    if (type == LightType::Directional)
    {
        lManager->addLight(base);
    }
    else if (type == LightType::Point)
    {
        PointLight* light = reinterpret_cast<PointLight*>(base.get());
        light->fallOut = fallout;
        light->radius = radius;
        lManager->addLight(base);
    }
    else 
    {
        SpotLight* light = reinterpret_cast<SpotLight*>(base.get());
        light->fallout = fallout;
        light->radius = radius;
        light->scale = scale;
        light->offset = offset;
        lManager->addLight(base);
    }
}

// =========================================================================================

OELightManager::OELightManager()
{
}

OELightManager::~OELightManager()
{
}

void OELightManager::calculatePointIntensity(float intensity, PointLight& light)
{
	light.intensity = intensity * static_cast<float>(M_1_PI) * 0.25f;
}

void OELightManager::calculateSpotIntensity(float intensity, float outerCone, float innerCone, SpotLight& spotLight)
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

void OELightManager::addLight(std::unique_ptr<LightBase>& light)
{
    switch (light->type)
	{
	case LightType::Spot:
	{
		SpotLight* sLight = static_cast<SpotLight*>(light.get());
		// carry out some of the calculations on the cpu side to save time
		calculateSpotIntensity(sLight->intensity, sLight->outerCone, sLight->innerCone, *sLight);
        break;
	}
	case LightType::Point:
	{
		PointLight* pLight = static_cast<PointLight*>(light.get());
		calculatePointIntensity(pLight->intensity, *pLight);
        break;
	}
    case LightType::Directional:
        break;
	}

	// nothing to be done with directional lights right now
	lights.emplace_back(std::move(light));
}

size_t OELightManager::getLightCount() const
{
	return lights.size();
}

LightBase* OELightManager::getLight(const size_t idx)
{
	assert(idx < lights.size());
	return lights[idx].get();
}

}    // namespace OmegaEngine
