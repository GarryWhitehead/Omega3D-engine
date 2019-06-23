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

		enum class TextureId
		{
			BaseColour,
			Emissive,
			MetallicRoughness,
			Normal,
			Occlusion,
			Count
		};

		struct Material
		{
			std::string name;

			struct Factors
			{
				OEMaths::vec3f emissive = OEMaths::vec3f{ 1.0f, 1.0f, 1.0f };
				OEMaths::vec4f baseColour = OEMaths::vec4f{ 1.0f, 1.0f, 1.0f, 1.0f };
				OEMaths::vec4f diffuse = OEMaths::vec4f{ 1.0f, 1.0f, 1.0f, 1.0f };
				OEMaths::vec3f specular = OEMaths::vec3f{ 0.0f, 0.0f, 0.0f };

				float specularGlossiness = 1.0f;
				float roughness = 1.0f;
				float metallic = 1.0f;

				std::string mask;
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
		int32_t getTexture(const TextureId id);

		Material::TexCoordSets getUvSets() const
		{
			return material.uvSets;
		}

		Material::Factors getFactors() const
		{
			return material.factors;
		}

		std::string& getAlphaMask()
		{
			return material.factors.mask;
		}

		float getAlphaCutOff() const
		{
			return material.factors.alphaMaskCutOff;
		}

		std::string& getName()
		{
			return material.name;
		}

		bool isUsingSpecularGlossiness() const
		{
			return material.usingSpecularGlossiness;
		}

	private:

		Material material;
	};

}

