#pragma once
#include "Models/GltfModel.h"
#include "Models/ModelMesh.h"
#include "OEMaths/OEMaths.h"
#include "models/OEModels.h"

#include <cstdint>
#include <memory>

namespace OmegaEngine
{
enum class ComponentType
{
	Transform,
	Mesh,
	Material,
	Texture,
	Skin,
	Skybox,
	ShadowMap,
	Skeleton,
	Joint,
	OEModel
};

struct ComponentBase
{
	virtual ~ComponentBase() = default;

	ComponentBase()
	{
	}
	ComponentBase(ComponentType _componentType)
	    : componentType(_componentType)
	{
	}

	ComponentType componentType;
};

struct WorldTransformComponent : public ComponentBase
{
	WorldTransformComponent()
	{
	}
	~WorldTransformComponent()
	{
	}

	WorldTransformComponent(uint32_t _index)
	    : index(_index)
	    , ComponentBase(ComponentType::Mesh)
	{
	}

	uint32_t index = 0;

	OEMaths::vec3f translation;
	OEMaths::vec3f scale;
	OEMaths::quatf rotation;
};

struct MeshComponent : public ComponentBase
{
	MeshComponent()
	{
	}
	~MeshComponent()
	{
	}

	MeshComponent(std::unique_ptr<ModelMesh> &_mesh, uint32_t offset)
	    : mesh(std::move(_mesh))
	    , materialBufferOffset(offset)
	    , ComponentBase(ComponentType::Mesh)
	{
	}

	uint32_t index = 0;
	uint32_t materialBufferOffset = 0;
	std::unique_ptr<ModelMesh> mesh;
};

struct TransformComponent : public ComponentBase
{
	TransformComponent(std::unique_ptr<ModelTransform> &_transform)
	    : transform(std::move(_transform))
	    , ComponentBase(ComponentType::Transform)
	{
	}

	uint32_t index = 0;
	uint32_t dynamicUboOffset = 0; // used by the renderable mesh
	std::unique_ptr<ModelTransform> transform;
};

struct SkinnedComponent : public ComponentBase
{
	SkinnedComponent(uint32_t _index, uint32_t offset)
	    : index(_index)
	    , bufferOffset(offset)
	    , ComponentBase(ComponentType::Skin)
	{
	}

	uint32_t index = 0;
	uint32_t bufferOffset = 0;
	uint32_t dynamicUboOffset = 0;
};

struct SkeletonComponent : public ComponentBase
{
	SkeletonComponent(uint32_t _index, uint32_t offset, bool _isRoot)
	    : index(_index)
	    , bufferOffset(offset)
	    , isRoot(_isRoot)
	    , ComponentBase(ComponentType::Skeleton)
	{
	}

	uint32_t index = 0;
	uint32_t bufferOffset = 0;
	bool isRoot = false;
};

struct MaterialComponent : public ComponentBase
{
	MaterialComponent(const std::string _name, const OEMaths::vec3f &_specular,
	                  const OEMaths::vec3f &_diffuse, OEMaths::vec4f &_baseColour,
	                  const float _roughness, const float _metallic)
	    : name(_name)
	    , specular(_specular)
	    , diffuse(_diffuse)
	    , baseColour(_baseColour)
	    , roughness(_roughness)
	    , metallic(_metallic)
	    , ComponentBase(ComponentType::Material)
	{
	}

	std::string name;
	OEMaths::vec3f specular;
	OEMaths::vec3f diffuse;
	OEMaths::vec4f baseColour;
	OEMaths::vec3f emissive;
	float roughness = 0.0f;
	float metallic = 0.0f;
	
	uint32_t offset = 0;
};

struct AnimationComponent : public ComponentBase
{
	AnimationComponent(uint32_t anim, std::vector<uint32_t> &channels, uint32_t offset)
	    : animIndex(anim)
	    , bufferOffset(offset)
	    , ComponentBase(ComponentType::Transform)
	{
		channelIndex = channels;
	}

	uint32_t animIndex = 0;
	std::vector<uint32_t> channelIndex;
	uint32_t bufferOffset = 0;
};

struct SkyboxComponent : public ComponentBase
{
	SkyboxComponent(float factor)
	    : blurFactor(factor)
	    , ComponentBase(ComponentType::Skybox)
	{
	}

	float blurFactor = 0.0f;
};

struct ShadowComponent : public ComponentBase
{
	ShadowComponent(float clamp, float constant, float slope)
	    : biasClamp(clamp)
	    , biasConstant(constant)
	    , biasSlope(slope)
	    , ComponentBase(ComponentType::ShadowMap)
	{
	}

	uint32_t index = 0;

	float biasClamp = 0.0f;
	float biasConstant = 0.0f;
	float biasSlope = 0.0f;
};

struct OEModelComponent : public ComponentBase
{
	OEModelComponent(Models::OEModels modelType)
	    : type(modelType)
	    , ComponentBase(ComponentType::OEModel)
	{
	}

	Models::OEModels type;
};

} // namespace OmegaEngine