#pragma once
#include "OEMaths/OEMaths.h"

#include "RenderableBase.h"

#include "Rendering/RenderInterface.h"

#include "VulkanAPI/Managers/BufferManager.h"
#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Interface.h"

#include "Types/PStateInfo.h"

#include "Managers/MaterialManager.h"

// Number of combined image sampler sets allowed for materials. This allows for materials to be added - this value will need monitoring
#define MAX_MATERIAL_SETS 50

// forward decleartions
namespace VulkanAPI
{
class Sampler;
class CommandBuffer;
class BufferManager;
class VkTextureManager;
}    // namespace VulkanAPI

namespace OmegaEngine
{
// forward declerations
class Object;
struct StaticMesh;
struct PrimitiveMesh;
class World;

// all material data required to draw
// storing this material data in two places for threading purposes.
struct MaterialPushBlock
{
	void prepare(MaterialInfo& mat)
	{
		// material factors
		baseColorFactor = mat.factors.baseColour;
		metallicFactor = mat.factors.metallic;
		roughnessFactor = mat.factors.roughness;
		emissiveFactor = mat.factors.emissive;
		specularFactor = mat.factors.specular;
		diffuseFactor =
		    OEMaths::vec3f{ mat.factors.diffuse.getX(), mat.factors.diffuse.getY(), mat.factors.diffuse.getZ() };

		// alpha blending
		alphaMask = (float)mat.alphaMask;
		alphaMaskCutoff = mat.alphaMaskCutOff;

		// whether this material has a particular property
		haveBaseColourMap = mat.hasTexture[(int)ModelMaterial::TextureId::BaseColour] ? 1 : 0;
		haveMrMap = mat.hasTexture[(int)ModelMaterial::TextureId::MetallicRoughness] ? 1 : 0;
		haveNormalMap = mat.hasTexture[(int)ModelMaterial::TextureId::Normal] ? 1 : 0;
		haveAoMap = mat.hasTexture[(int)ModelMaterial::TextureId::Occlusion] ? 1 : 0;
		haveEmissiveMap = mat.hasTexture[(int)ModelMaterial::TextureId::Emissive] ? 1 : 0;
		usingSpecularGlossiness = mat.usingSpecularGlossiness ? 1 : 0;

		// sets to use
		baseColourUvSet = mat.uvSets.baseColour;
		metallicRoughnessUvSet = mat.uvSets.metallicRoughness;
		normalUvSet = mat.uvSets.normal;
		occlusionUvSet = mat.uvSets.occlusion;
		emissiveUvSet = mat.uvSets.emissive;

		// if using specular glossiness property, overright roughness and colour with gloss props
		if (usingSpecularGlossiness)
		{
			metallicRoughnessUvSet = mat.uvSets.specularGlossiness;
			baseColourUvSet = mat.uvSets.diffuse;
		}
	}

	// colour factors
	OEMaths::vec4f baseColorFactor;
	OEMaths::vec3f emissiveFactor;
	float pad0;
	OEMaths::vec3f diffuseFactor;
	float pad1;
	OEMaths::vec3f specularFactor;
	float pad2;
	float metallicFactor;
	float roughnessFactor;

	// alpha blending
	float alphaMask;
	float alphaMaskCutoff;

	// uv sets for each texture
	uint32_t baseColourUvSet;
	uint32_t metallicRoughnessUvSet;
	uint32_t normalUvSet;
	uint32_t emissiveUvSet;
	uint32_t occlusionUvSet;

	// indication whether we have a ceratin texture map
	uint32_t haveBaseColourMap;
	uint32_t haveNormalMap;
	uint32_t haveEmissiveMap;
	uint32_t haveMrMap;
	uint32_t haveAoMap;
	uint32_t usingSpecularGlossiness;
};


// renderable object types
class RenderableMesh : public RenderableBase
{

public:
	// render info that will be used to draw this mesh
	struct MeshInstance
	{
		StateMesh type;

		// pipeline
		ProgramState* state = nullptr;

		// per primitive index data
		uint32_t indexPrimitiveOffset;    // this equates to buffer_offset + sub-offset
		uint32_t indexPrimitiveCount;

		// the starting offsets within the main vertices/indices buffer
		uint32_t indexOffset;    // index into large buffer
		uint32_t vertexOffset;

		// vertex and index buffer memory info
		VulkanAPI::Buffer vertexBuffer;
		VulkanAPI::Buffer indexBuffer;

		MaterialPushBlock pushBlock;

		// vulkan stuff for material textures
		VulkanAPI::DescriptorSet descriptorSet;

		// offset into transform buffer for this mesh
		uint32_t transformDynamicOffset = 0;
		uint32_t skinnedDynamicOffset = 0;
	};

	void* getHandle() override
	{
		return this;
	}

	
	RenderableMesh();

	void prepare(World& world, StaticMesh& mesh, PrimitiveMesh& primitive, Object& obj);

	void preparePStateInfo(StateId::StateFlags& flags);

	void render(VulkanAPI::SecondaryCommandBuffer& cmdBuffer, void* instanceData) override;

private:

	PStateInfo info;
};
}    // namespace OmegaEngine
