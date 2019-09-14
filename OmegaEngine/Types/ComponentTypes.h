#pragma once

#include "OEMaths/OEMaths.h"

#include <cstdint>
#include <memory>

namespace OmegaEngine
{

struct ComponentBase
{
	virtual ~ComponentBase() = default;

	ComponentBase() = default;
	
};

struct WorldTransformComponent : public ComponentBase
{
	WorldTransformComponent() = default;

	/**
	 * Sets the translation of the world transform.
	 * @param trans: a vector containing the translation co-ords
	 */
	void setTranslation(const OEMaths::vec3f& trans)
	{
		translation = trans;
	}

	/**
	 * Sets the scale of the world transform.
	 * @param s: a vector containing the amount to scale each axis
	 */
	void setScale(const OEMaths::vec3f& s)
	{
		scale = s;
	}

	/**
	 * Sets the rotation of the world transform.
	 * @param r: a quaternoin containing the amount to rotate each axis
	 */
	void setRotation(const OEMaths::quatf& r)
	{
		rotation = r;
	}

	OEMaths::vec3f translation;
	OEMaths::vec3f scale;
	OEMaths::quatf rotation;
};

struct MeshComponent : public ComponentBase
{
	MeshComponent() = default;

	/**
	 * The index of this skin component within the transform manager conatiner
	 * @param index: the index within the manager - set through the builders
	 */
	void setIndex(const size_t i)
	{
		index = i;
	}

	size_t index = 0;
	int32_t materialBufferOffset = -1;	// is this used?
};

struct TransformComponent : public ComponentBase
{
	TransformComponent() = default;

	size_t index = 0;
	size_t dynamicUboOffset = 0; // used by the renderable mesh

	/**
	 * sets the combined local matrix transform
	 * @param localTransform: the local matrix.
	 */
	void setTransform(const OEMaths::mat4f& trans)
	{
		transform = trans;
	}

	OEMaths::vec3f translation;
	OEMaths::vec3f scale;
	OEMaths::quatf rotation;
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
	void setIndex(const size_t index, const size_t offset)
	{
		index = index + offset;
	}

	size_t index = 0;
	size_t dynamicUboOffset = 0;	// not set by the user
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
	{
	}

	std::string name;
	OEMaths::vec3f specular;
	OEMaths::vec3f diffuse;
	OEMaths::vec4f baseColour;
	OEMaths::vec3f emissive;
	float roughness = 0.0f;
	float metallic = 0.0f;
	
	size_t offset = 0;
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

	size_t index = 0;

	float biasClamp = 0.0f;
	float biasConstant = 0.0f;
	float biasSlope = 0.0f;
};

} // namespace OmegaEngine