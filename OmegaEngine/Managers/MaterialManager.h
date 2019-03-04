#pragma once

#include "Managers/ManagerBase.h"
#include "OEMaths/OEMaths.h"

#include "Vulkan/Descriptors.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Vulkan/Sampler.h"

#include "tiny_gltf.h"

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
				std::tuple<uint32_t, int> image;	// set number and the index within this set
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

			// local vulkan data
			std::array<VulkanAPI::Texture, static_cast<int>(PbrMaterials::Count) > vk_textures;
			VulkanAPI::DescriptorSet descr_set;
			VulkanAPI::Sampler sampler;

			bool usingExtension = false;
		};

		MaterialManager(vk::Device dev);
		~MaterialManager();

		void update_frame(double time, double dt, 
							std::unique_ptr<ObjectManager>& obj_manager,
							std::unique_ptr<ComponentInterface>& component_interface) override;

		void addGltfMaterial(tinygltf::Material& gltf_mat, TextureManager& textureManager);
		MaterialInfo& get(uint32_t index);

	private:
		
		// for the updating of materials
		vk::Device device;

		std::vector<MaterialInfo> materials;
		bool isDirty = true;
	};

}

