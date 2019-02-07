#pragma once

#include "Managers/ManagerBase.h"
#include "OEMaths/OEMaths.h"
#include "Omega_Common.h"

#include <memory>
#include <unordered_map>

namespace OmegaEngine
{
	// forward declerations
	class TextureManager;

	enum class PbrMaterials 
	{
		BaseColor,
		MetallicRoughness,
		Normal,
		Emissive,
		Occlusion,
		Count
	};

	class MaterialManager : public ManagerBase
	{

	public:

		struct MaterialInfo
		{
			enum class AlphaMode
			{
				Blend,
				Mask,
				None
			};

			struct Texture
			{
				uint32_t sampler;
				uint32_t image;
			};

			AlphaMode alphaMode = AlphaMode::None;
			struct Factors
			{
				OEMaths::vec3f emissive;
				float specularGlossiness;
				float baseColour;
				float roughness;
				float diffuse;
				float metallic;
				float specular;
				float alphaMask;
				float alphaMaskCutOff;
			} factors;

			// material image indicies
			std::array<Texture, static_cast<int>(PbrMaterials::Count) > textures;

			bool usingExtension = false;
		};

		MaterialManager();
		~MaterialManager();

		void addGltfMaterial(tinygltf::Material& gltf_mat, TextureManager& textureManager);
		MaterialInfo& get(uint32_t index);

	private:

		std::vector<MaterialInfo> materials;

	};

}

