#pragma once

#include "OEMaths/OEMaths.h"

#include <cstdint>

namespace OmegaEngine
{

class IblImage
{
public:

	// BDRF integration
	OEMaths::vec2f integrateBRDF(float NdotV, float roughness, uint16_t sampleCount);

	// irradiance map
	void prepareIrradianceMap(float dPhi, float dTheta);

	// pre-filtered map
	void preparePreFiltered(const uint16_t roughnessFactor, const uint16_t sampleCount);

private:

	OEMaths::vec2f hammersley(uint64_t i, uint64_t N);
	OEMaths::vec3f GGX_ImportanceSample(OEMaths::vec2f Xi, OEMaths::vec3f N, float roughness);
	float geometryShlickGGX(float NdotV, float NdotL, float roughness);
	float GGX_Distribution(float NdotH, float roughness);

private:
};

}    // namespace OmegaEngine
