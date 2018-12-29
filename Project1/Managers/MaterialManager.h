#pragma once

#include <memory>
#include <unordered_map>

#include "OEMaths/OEMaths.h"
#include "tiny_gltf.h"

namespace OmegaEngine
{
	// forward declerations
	class TextureManager;

	class MaterialManager
	{

	public:

		// in a format for sending immediately to the shader as a push if using vulkan as a renderer
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
		};

		struct MaterialInfo
		{
			enum class AlphaMode
			{
				Blend,
				Mask,
				None
			};

			AlphaMode alphaMode = AlphaMode::None;
			MaterialPushBlock pushBlock;

			// material image indicies
			int baseColorIndex;
			int metallicRoughnessIndex;
			int normalIndex;
			int emissiveIndex;
			int occlusionIndex;

			bool usingExtension = false;
		};

		MaterialManager();
		~MaterialManager();

		void addGltfMaterial(tinygltf::Material& gltf_mat);

	private:

		std::unordered_map<const char*, MaterialInfo> materials;

		std::unique_ptr<TextureManager> textureManager;
	};

}

