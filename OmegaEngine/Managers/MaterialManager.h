#pragma once

#include "Managers/ManagerBase.h"
#include "OEMaths/OEMaths.h"

#include "Vulkan/Descriptors.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/Queue.h"

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
		Normal,
		MetallicRoughness,
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

			AlphaMode alphaMode = AlphaMode::None;
			struct Factors
			{
				OEMaths::vec3f emissive = OEMaths::vec3f{ 0.0f, 0.0f, 0.0f };
				float specularGlossiness = 1.0f;
				float baseColour = 1.0f;
				float roughness = 1.0f;
				float diffuse = 1.0f;
				float metallic = 1.0f;
				float specular = 1.0f;
				float alphaMask = (float)AlphaMode::Opaque;
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

			// material image indicies
			std::array<Texture, static_cast<int>(PbrMaterials::Count)> textures;
			std::array<bool, static_cast<int>(PbrMaterials::Count)> texture_state = { false };

			// if using specular glossiness then color and metallic/roughness texture indicies will be automatically changed for this workflow
			bool usingSpecularGlossiness = false;

			// local vulkan data
			std::array<VulkanAPI::Texture, static_cast<int>(PbrMaterials::Count) > vk_textures;
			VulkanAPI::DescriptorSet descr_set;
			VulkanAPI::Sampler sampler;
		};

		MaterialManager(vk::Device& dev, vk::PhysicalDevice& phys_device, VulkanAPI::Queue& queue);
		~MaterialManager();

		// a per-frame update if the material data becomes dirty
		void update_frame(double time, double dt, std::unique_ptr<ObjectManager>& obj_manager, ComponentInterface* component_interface) override;

		void addGltfMaterial(uint32_t set, tinygltf::Material& gltf_mat, TextureManager& textureManager);
		MaterialInfo& get(uint32_t index);

		void add_descr_layout(vk::DescriptorSetLayout& layout, vk::DescriptorPool& pool)
		{
			descr_layout = layout;
			descr_pool = pool;
		}

	private:
		
		// for the updating of materials
		vk::Device device;
		vk::PhysicalDevice gpu;
		VulkanAPI::Queue graph_queue;

		// a pointer to the descr set - init in the mesh pipeline and needed for creating the set here
		vk::DescriptorSetLayout descr_layout;
		vk::DescriptorPool descr_pool;

		std::vector<MaterialInfo> materials;
		bool isDirty = true;
	};

}

