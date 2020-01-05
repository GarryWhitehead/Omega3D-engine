#pragma once

#include "OEMaths/OEMaths.h"

#include <cstdint>
#include <vector>

namespace OmegaEngine
{

// forward declerations
class Object;

class ModelGraph
{
public:
    
    struct Transform
    {
        // world transform
        OEMaths::vec3f trans;
        OEMaths::vec3f scale;
        OEMaths::quatf rot;
        OEMaths::mat4f worldMat;
        
        // the final matrix - world * local
        OEMaths::mat4f matrix;
    };
    
    struct Node
    {
        Object* parent = nullptr;
        std::vector<Object*> children;
        
        Transform world;
    };
    
    void addNode(Object* parentObj);
    bool addChildNode(Object* parentObj, Object* childObj);
    bool deleteChildNode(Object* parentObj, Object* childObj);
    bool deleteNode(Object* parentObj);
    
    friend class Scene;
    
private:
    
    std::vector<Node*> nodes;
    
};

}
