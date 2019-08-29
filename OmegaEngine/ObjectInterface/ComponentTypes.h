#pragma once
#include "Models/Gltf/GltfModel.h"
#include "Models/ModelMesh.h"
#include "OEMaths/OEMaths.h"
#include "Models/OEModels.h"

#include <cstdint>
#include <memory>

namespace OmegaEngine
{

struct ComponentBase
{
	virtual ~ComponentBase() = default;

	ComponentBase()
	{
	}
	
};

struct WorldTransformComponent : public ComponentBase
{
	WorldTransformComponent()
	{
	}
	~WorldTransformComponent()
	{
	}

	WorldTransformComponent(const OEMaths::vec3f& t, const OEMaths::vec3f& s,
	                        const OEMaths::quatf& r)
	    : translation(t)
	    , scale(s), 
		rotation(r)
	    , ComponentBase(ComponentType::WorldTransform)
	{
	}

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

	MeshComponent(std::unique_ptr<ModelMesh> &_mesh)
	    : mesh(std::move(_mesh))
		, ComponentBase(ComponentType::Mesh)
	{
	}

	uint32_t index = 0;
	int32_t materialBufferOffset = -1;
	std::unique_ptr<ModelMesh> mesh;
};

struct TransformComponent : public ComponentBase
{
	TransformComponent() = default;

	uint32_t index = 0;
	uint32_t dynamicUboOffset = 0; // used by the renderable mesh

	/**
	 * sets the combined local matrix transform
	 * @param localTransform: the local matrix.
	 */
	void setTransform(const OEMaths::mat4f& trans)
	{
		transform = trans;
	}

	OEMaths::mat4f trasform;
};

struct SkinnedComponent : public ComponentBase
{
	SkinnedComponent() = default;

	/**
	 * The index of this skin component within the transform manager conatiner
	 * @param index: the index within the particular of group of skins. For instance, this may
	 * be associated with a group of GLTF skins whcih are being added
	 * @param offset: The index offset in the transform manager buffer in which these group of skins
	 * begin at.
	 */
	void setIndex(const uint32_t index, const uint32_t offset)
	{
		animIndex = index + offset;
	}

	uint32_t index = 0;
	uint32_t dynamicUboOffset = 0;	// not set by the user
};

struct SkeletonComponent : public ComponentBase
{
	SkeletonComponent() = default;

	/**
	 * The index of this skeleton component within the transform manager conatiner
	 * @param index: the index within the particular of group of skeletons. For instance, this may
	 * be associated with a group of GLTF skeletons whcih are being added
	 * @param offset: The index offset in the transform manager buffer in which these group of skeletons
	 * begin at.
	 */
	void setIndex(const uint32_t index, const uint32_t offset)
	{
		animIndex = index + offset;
	}

	/**
	 *  Specifies whether this skeleton is the root
	 * @param isRoot: a boolean indicating if this comp is the root
	 */
	void setRoot(const bool root)
	{
		isRoot = root;
	}

	uint32_t index = 0;
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
	AnimationComponent() = default;

	/**
	 * The index of this animation component within the anim manager conatiner
	 * @param index: the index within the particular of group of animations. For instance, this may
	 * be associated with a group of GLFF animations whcih are being added
	 * @param offset: The index offset in the anim manager buffer in which these group of anim
	 * begin at.
	 */
	void setIndex(const uint32_t index, const uint32_t offset)
	{
		animIndex = index + offset;
	}

	/**
	 * A group of channel indices which this animation refers too
	 * @param indices: a vector of channel index values
	 */
	void setChannelIndex(const std::vector<uint32_t>& indices)
	{
		channelIndex = indicies;
	}

	uint32_t animIndex = 0;
	std::vector<uint32_t> channelIndex;
};

struct SkyboxComponent : public ComponentBase
{
	SkyboxComponent() = default;
	
	/**
	 * Sets the blur factor to add to the skybox. Lower values create a more blurred
	 * look. 
	 * @param factor: the amount by which to blur the skybox by. Clamped between 0 and 1
	 */
	void setBlurFactor(const float factor)
	{
		float clamped = std::clamp(factor, 0.0f, 1.0f);
		blurFactor = clamped;
	}

	float blurFactor = 0.0f;
};

struct ShadowComponent : public ComponentBase
{
	ShadowComponent() = default;

	/**
	 * Sets the clamped bias level when using mapped shadows
	 * @param clamp:
	 */
	void setBiasClamp(const float clamp)
	{
		biasClamp = clamp;
	}

	/**
	 * Sets the constant bias level when using mapped shadows
	 * @param constant:
	 */
	void setBiasConstant(const float constant)
	{
		biasConstant = constant;
	}

	/**
	 * Sets the slope bias level when using mapped shadows
	 * @param slope:
	 */
	void setBiasSlope(const float slope)
	{
		biasSlope = slope;
	}

	uint32_t index = 0;

	float biasClamp = 0.0f;
	float biasConstant = 0.0f;
	float biasSlope = 0.0f;
};

} // namespace OmegaEngine