#pragma once

#include "Managers/ComponentInterface.h"

namespace OmegaEngine
{
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
        SkyboxComponent() : ComponentBase(ManagerType::None) {}
    };
}