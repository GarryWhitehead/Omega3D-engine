#pragma once

#include <memory>

#include "tiny_gltf.h"
#include "glm.hpp"

namespace OmegaEngine
{
	// forward declerations
	class TextureManager;

	class MaterialManager
	{

	public:

		// in a format for sending immediately to the shader as a push
		struct MaterialPushBlock
		{
			float emissiveFactor;
			float baseColourFactor;
			float roughnessFactor;
			float diffuseFactor;
			float metallicFactor;
			float specularFactor;
			float alphaMask;
			float alphaMaskCutOff;

		};

		struct MaterialInfo
		{
			// all the info required by the shader for PBR rendering
			MaterialPushBlock pushBlock;

		};

		MaterialManager();
		~MaterialManager();

		void addData();

	private:

		std::unordered_map<uint32_t, std::vector<Material> > materials;

		std::unique_ptr<TextureManager> textureManager;
	};

}

