#pragma once

#include <memory>

#include "tiny_gltf.h"
#include "glm.hpp"

namespace OmegaEngine
{

	enum class AlphaMode
	{
		Blend,
		Mask,
		None
	};

	class MaterialManager
	{

	public:

		struct MaterialExt
		{
			int specularGlossiness;
			int diffuse;
			float specularGlossinessFactor = 0.0f;
			float diffuseFactor = 0.0f;
		};

		struct Material
		{
			float roughnessFactor = 0.0f;
			float metallicFactor = 0.0f;
			float baseColorFactor = 0.0f;
			glm::vec3 emissiveFactor = glm::vec3(0.0f);

			AlphaMode alphaMode = AlphaMode::None;
			float alphaCutOff = 0.0f;

			struct TextureIndex
			{
				int baseColor;
				int metallicRoughness;
				int normal;
				int emissive;
				int occlusion;
			} textureIndicies;

			std::unique_ptr<MaterialExt> extension;

		};

		MaterialManager();
		~MaterialManager();

		void parseGltfFile(uint32_t spaceId, tinygltf::Model& model);

	private:

		std::unordered_map<uint32_t, std::vector<Material> > materials;
	};

}

