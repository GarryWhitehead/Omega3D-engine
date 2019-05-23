#pragma once

#include "Objects/ObjectTypes.h"

namespace OmegaEngine
{
	enum class ManagerType
	{
		None,
		Mesh,
		Material,
		Texture,
		Camera,
		Light,
		Transform
	};


	struct ComponentBase
    {
		virtual ~ComponentBase() = default;

		ComponentBase() {}
		ComponentBase(ManagerType _type) :
            type(_type)
        {}

        ManagerType type;
    };

    struct MeshComponent : public ComponentBase
    {
		MeshComponent() {}
		~MeshComponent() {}

		MeshComponent(uint32_t _index) : 
            index(_index),
            ComponentBase(ManagerType::Mesh) 
        {}

        uint32_t index = 0;
    };

    struct TransformComponent : public ComponentBase
    {
        TransformComponent(uint32_t _index) : 
            index(_index),
            ComponentBase(ManagerType::Transform)
        {}

        uint32_t index = 0;
    };

    struct SkinnedComponent : public ComponentBase
    {
        SkinnedComponent(uint32_t _index) : 
            index(_index),
            ComponentBase(ManagerType::Transform)
        {}
        
        uint32_t index = 0;
    };

    struct MaterialComponent : public ComponentBase
    {
        MaterialComponent(uint32_t _index) : 
        index(_index),
        ComponentBase(ManagerType::Material) 
        {}

        uint32_t index = 0;
    };

    struct SkyboxComponent : public ComponentBase
    {
        SkyboxComponent(float factor) : 
			blurFactor(factor),
			ComponentBase(ManagerType::None) {}

		float blurFactor = 0.0f;
    };

	struct ShadowComponent : public ComponentBase
	{
		ShadowComponent(float clamp, float constant, float slope) :
			biasClamp(clamp),
			biasConstant(constant),
			biasSlope(slope),
			ComponentBase(ManagerType::Mesh) {}

		uint32_t index = 0;

		float biasClamp = 0.0f;
		float biasConstant = 0.0f;
		float biasSlope = 0.0f;
	};
}