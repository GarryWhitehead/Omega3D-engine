#pragma once
#include "OEMaths/OEMaths.h"
#include "Managers/MaterialManager.h"

namespace VulkanAPI
{
	
	struct MaterialPushBlock
	{
		OEMaths::vec3f emissiveFactor;
		float specularGlossinessFactor;
		float baseColourFactor;
		float roughnessFactor;
		float diffuseFactor;
		float metallicFactor;
		float specularFactor;
		float alphaMask;
		float alphaMaskCutOff;

		void create(OmegaEngine::MaterialManager::MaterialInfo mat)
		{

		}
	};
}