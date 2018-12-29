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
			int32_t baseColorIndex;
			int32_t metallicRoughnessIndex;
			int32_t normalIndex;
			int32_t emissiveIndex;
			int32_t occlusionIndex;

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

