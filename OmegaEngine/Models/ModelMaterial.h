#pragma once
#include "OEMaths/OEMaths.h"

#include <cstdint>
#include <string>
#include "tiny_gltf.h"

namespace OmegaEngine
{

	class ModelMaterial
	{

	public:

		struct Material
		{
			std::string name;

			enum class AlphaMode
			{
				Opaque,
				Blend,
				Mask
			};

			struct Texture
			{
				uint32_t set;
				uint32_t sampler = 0;
				uint32_t image = 0;			// set number and the index within this set
			};

			struct Factors
			{
				OEMaths::vec3f emissive = OEMaths::vec3f{ 0.0f, 0.0f, 0.0f };
				OEMaths::vec4f baseColour = OEMaths::vec4f{ 1.0f, 1.0f, 1.0f, 1.0f };
				OEMaths::vec4f diffuse = OEMaths::vec4f{ 1.0f, 1.0f, 1.0f, 1.0f };
				OEMaths::vec3f specular = OEMaths::vec3f{ 0.0f, 0.0f, 0.0f };

				float specularGlossiness = 1.0f;
				float roughness = 1.0f;
				float metallic = 1.0f;

				AlphaMode alphaMask = AlphaMode::Opaque;
				float alphaMaskCutOff = 1.0f;
			} factors;

			struct TexCoordSets
			{
				uint32_t baseColour = 0;
				uint32_t metallicRoughness = 0;
				uint32_t normal = 0;
				uint32_t emissive = 0;
				uint32_t occlusion = 0;
				uint32_t specularGlossiness = 0;
				uint32_t diffuse = 0;
			} uvSets;

			struct TextureIndex
			{
				int32_t baseColour = -1;
				int32_t emissive = -1;
				int32_t metallicRoughness = -1;
				int32_t normal = -1;
				int32_t occlusion = -1;
			} textures;

			// if using specular glossiness then color and metallic/roughness texture indicies will be automatically changed for this workflow
			bool usingSpecularGlossiness = false;
		};

		ModelMaterial();
		~ModelMaterial();

		void extractMaterialData(tinygltf::Material& gltfMaterial);
		void addGltfSampler(tinygltf::Sampler& gltf_sampler);

		

	private:

		Material material;
	};

}

