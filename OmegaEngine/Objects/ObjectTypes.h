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
        ComponentBase(ManagerType _type) :
            type(_type)
        {}

        ManagerType type;
    };

    struct MeshComponent : public ComponentBase
    {
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
			blur_factor(factor),
			ComponentBase(ManagerType::None) {}

		float blur_factor = 0.0f;
    };

	struct ShadowComponent : public ComponentBase
	{
		ShadowComponent(uint32_t _index, float clamp, float constant, float slope) :
			index(_index),
			bias_clamp(clamp),
			bias_constant(constant),
			bias_slope(slope),
			ComponentBase(ManagerType::Mesh) {}

		uint32_t index = 0;

		float bias_clamp = 0.0f;
		float bias_constant = 0.0f;
		float bias_slope = 0.0f;
	};
}