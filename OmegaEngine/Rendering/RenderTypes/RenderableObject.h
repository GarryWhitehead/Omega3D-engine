#pragma once

#include "OEMaths/OEMaths.h"

#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Managers/BufferManager.h"

#include "Types/PBuildInfo.h"

// Number of combined image sampler sets allowed for materials.
#define MAX_MATERIAL_SETS 50

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

		// if using specular glossiness property, overright roughness and colour with gloss props
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

	uint32_t usingSpecularGlossiness;
};


// renderable object types
class RenderableObject
{

public:

	enum class RenderableType
	{
		Static,
		Skinned,
		Shadow
	};

	struct ShadowInfo
	{
		size_t lightAlignmentSize = 0;
		size_t lightCount = 0;

		float biasConstant = 0.0f;
		float biasClamp = 0.0f;
		float biasSlope = 0.0f;
	};


	RenderableObject();
	~RenderableObject();

	// not copyable but moveable
	RenderableObject(const RenderableObject&) = delete;
	RenderableObject& operator=(const RenderableObject&) = delete;
	RenderableObject(RenderableObject&&) = default;
	RenderableObject& operator=(RenderableObject&&) = default;

	void prepare(World& world, Object& obj);
	void prepareMaterial(ModelMaterial& mat);

	void preparePStateInfo(StateId::StateFlags& flags);

	void render(VulkanAPI::SecondaryCommandBuffer& cmdBuffer, void* instanceData) override;

	void* getHandle() override
	{
		return this;
	}

private:

	RenderableType type;

	/// determines sort order of renderable
	SortKey sortKey;

	/// how to render this renderable
	QueueType queueType;

	/// pipeline
	ProgramState* state = nullptr;

	/// per primitive index data
	uint32_t indexPrimitiveOffset;    // this equates to buffer_offset + sub-offset
	uint32_t indexPrimitiveCount;

	/// the starting offsets within the main vertices/indices buffer
	uint32_t indexOffset;    // index into large buffer
	uint32_t vertexOffset;

	/// vertex and index buffer memory info
	VulkanAPI::Buffer vertexBuffer;
	VulkanAPI::Buffer indexBuffer;

	/// material data - in a format ready to be pushed to the shader
	/// this is null if the renderable is for shadows or the mesh has no material
	MaterialPushBlock* material = nullptr;

	/// shadow data
	ShadowInfo* shadowInfo = nullptr;

	/// vulkan stuff for material textures
	VulkanAPI::DescriptorSet descriptorSet;

	/// offset into transform buffer for this mesh
	uint32_t transformDynamicOffset = 0;
	uint32_t skinnedDynamicOffset = 0;

	/// defines how to build the pipeline for this renderable
	PBuildInfo info;
};
}    // namespace OmegaEngine
