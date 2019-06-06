#pragma once
#include "OEMaths/OEMaths.h"

#include <cstdint>
#include <string>
#include "Models/MappedTexture.h"
#include "tiny_gltf.h"

namespace OmegaEngine
{

	class ModelMaterial
	{

	public:

		enum class TextureFormat
		{
			Image8UC4,
			Image16UC4,
			ImageBC3
		};

		struct Sampler
		{
			Sampler(vk::SamplerAddressMode _mode, vk::Filter _filter) :
				mode(_mode),
				filter(_filter)
			{}

			vk::SamplerAddressMode mode;
			vk::Filter filter;
		};

		struct Texture
		{
			Texture(std::string& _name) :
				name(_name)
			{}

			std::string name;

			uint8_t* imageData;
			uint32_t width;
			uint32_t height;

			// usually 4 channel RGBA
			TextureFormat format;

			// nullptr if not specfied in the gltf file
			std::unique_ptr<Sampler> sampler;

			void map(uint32_t width, uint32_t height, void* data)
			{
				assert(data != nullptr);
				uint32_t size = width * height * 4;
				memcpy(imageData, data, size);
				assert(imageData != nullptr);

				// images must be 4-channel RGBA
				format = TextureFormat::Image8UC4;
			}
		};

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

		void extractfImageData(tinygltf::Model& model);
		void extractMaterialData(tinygltf::Model& model, tinygltf::Material& gltfMaterial);
		void addGltfSampler(tinygltf::Sampler& gltf_sampler);

		static vk::SamplerAddressMode getWrapMode(int32_t wrap);
		static vk::Filter getFilterMode(int32_t filter);

	private:

		Material material;

		std::vector<Texture> textures;
	};

}

