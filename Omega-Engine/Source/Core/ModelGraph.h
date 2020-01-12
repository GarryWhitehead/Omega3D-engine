#pragma once

#include "OEMaths/OEMaths.h"

#include <cstdint>
#include <vector>

namespace OmegaEngine
{

// forward declerations
class OEObject;

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
        OEObject* parent = nullptr;
        std::vector<OEObject*> children;
        
        Transform world;
    };
    
    void addNode(OEObject* parentObj);
    bool addChildNode(OEObject* parentObj, OEObject* childObj);
    bool deleteChildNode(OEObject* parentObj, OEObject* childObj);
    bool deleteNode(OEObject* parentObj);
    
    friend class OEScene;
    
private:
    
    std::vector<Node*> nodes;
    
};

}
