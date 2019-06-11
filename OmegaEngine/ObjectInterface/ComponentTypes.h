#pragma once
#include "Models/ModelMesh.h"
#include "Models/GltfModel.h"
#include "OEMaths/OEMaths.h"

#include <cstdint>

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
		ShadowMap
	};


	struct ComponentBase
	{
		virtual ~ComponentBase() = default;

		ComponentBase() {}
		ComponentBase(ComponentType _componentType) :
			componentType(_componentType)
		{}

		ComponentType componentType;
	};

	struct WorldTransformComponent : public ComponentBase
	{
		WorldTransformComponent() {}
		~WorldTransformComponent() {}

		WorldTransformComponent(uint32_t _index) :
			index(_index),
			ComponentBase(ComponentType::Mesh)
		{}

		uint32_t index = 0;

		OEMaths::vec3f translation;
		OEMaths::vec3f scale;
		OEMaths::quatf rotation;
	};

    struct MeshComponent : public ComponentBase
    {
		MeshComponent() {}
		~MeshComponent() {}

		MeshComponent(ModelMesh* _mesh, uint32_t offset) : 
            mesh(_mesh),
			materialBufferOffset(offset),
            ComponentBase(ComponentType::Mesh) 
        {}

        uint32_t index = 0;
		uint32_t materialBufferOffset = 0;
		ModelMesh* mesh;
    };

    struct TransformComponent : public ComponentBase
    {
        TransformComponent(uint32_t _index) : 
            index(_index),
            ComponentBase(ComponentType::Transform)
        {}

        uint32_t index = 0;
		ModelTransform* transform;
    };

    struct SkinnedComponent : public ComponentBase
    {
        SkinnedComponent(uint32_t _index, bool skeleton, bool joint) : 
            index(_index),
			isSkeleton(skeleton),
			isJoint(joint),
            ComponentBase(ComponentType::Skin)
        {}
        
        uint32_t index = 0;
		uint32_t bufferOffset = 0;
		bool isSkeleton = false;
		bool isJoint = false;
    };

    struct MaterialComponent : public ComponentBase
    {
        MaterialComponent(uint32_t _offset) : 
        offset(_offset),
        ComponentBase(ComponentType::Material) 
        {}

        uint32_t offset = 0;
    };

    struct SkyboxComponent : public ComponentBase
    {
        SkyboxComponent(float factor) : 
			blurFactor(factor),
			ComponentBase(ComponentType::Skybox) {}

		float blurFactor = 0.0f;
    };

	struct ShadowComponent : public ComponentBase
	{
		ShadowComponent(float clamp, float constant, float slope) :
			biasClamp(clamp),
			biasConstant(constant),
			biasSlope(slope),
			ComponentBase(ComponentType::ShadowMap) {}

		uint32_t index = 0;

		float biasClamp = 0.0f;
		float biasConstant = 0.0f;
		float biasSlope = 0.0f;
	};
}