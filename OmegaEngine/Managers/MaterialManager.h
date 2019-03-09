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
				Blend,
				Mask,
				None
			};

			struct Texture
			{
				uint32_t set;
				uint32_t sampler;
				uint32_t image;	// set number and the index within this set
			};

			AlphaMode alphaMode = AlphaMode::None;
			struct Factors
			{
				OEMaths::vec3d emissive;
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
			std::array<Texture, static_cast<int>(PbrMaterials::Count)> textures;
			std::array<bool, static_cast<int>(PbrMaterials::Count)> texture_state = { false };

			// local vulkan data
			std::array<VulkanAPI::Texture, static_cast<int>(PbrMaterials::Count) > vk_textures;
			VulkanAPI::DescriptorSet descr_set;
			VulkanAPI::Sampler sampler;

			bool usingExtension = false;
		};

		MaterialManager(vk::Device& dev, vk::PhysicalDevice& phys_device, VulkanAPI::Queue& queue);
		~MaterialManager();

		void update_frame(double time, double dt, 
							std::unique_ptr<ObjectManager>& obj_manager,
							ComponentInterface* component_interface) override;

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

