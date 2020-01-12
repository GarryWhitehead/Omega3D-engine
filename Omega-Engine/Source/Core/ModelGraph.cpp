
#include "ModelGraph.h"

#include "Core/ObjectManager.h"

#include "utility/Logger.h"

namespace OmegaEngine
{

void ModelGraph::addNode(OEObject* parentObj)
{
    assert(parentObj);
    Node* node = new Node;
    node->parent = parentObj;
}

bool ModelGraph::addChildNode(OEObject* parentObj, OEObject* childObj)
{
    if (nodes.empty())
    {
        LOGGER_WARN("Trying to insert a child into an empty model graph");
        return false;
    }
    
    for (auto* node : nodes)
    {
        if (node->parent->getId() == parentObj->getId())
        {
            node->children.emplace_back(childObj);
            return true;
        }
    }
    
    LOGGER_ERROR("Parent with id of %llu does not exsist within the model graph", parentObj->getId());
    return false;
}

bool ModelGraph::deleteChildNode(OEObject* parentObj, OEObject* childObj)
{
    if (nodes.empty())
    {
        LOGGER_WARN("Trying to idelete a child when the model graph is empry");
        return false;
    }
    
    for (auto* node : nodes)
    {
        if (node->parent->getId() == parentObj->getId())
        {
            size_t count = 0;
            for (auto* child : node->children)
            {
                if (child->getId() == childObj->getId())
                {
                    // this is an expensive operation - should use a different container
                    node->children.erase(node->children.begin() + count);
                    return true;
                }
            }
            ++count;
        }
        
        LOGGER_ERROR("Chiild with id of %llu does not exsist within the model graph", childObj->getId());
        return false;
    }
    
    LOGGER_ERROR("Parent with id of %llu does not exsist within the model graph", parentObj->getId());
    return false;
}

bool ModelGraph::deleteNode(OEObject* parentObj)
{
    if (nodes.empty())
    {
       LOGGER_WARN("Trying to idelete a child when the model graph is empry");
       return false;
    }
       
    for (auto* node : nodes)
    {
        size_t count = 0;
        if (node->parent->getId() == parentObj->getId())
        {
            // this is an expensive operation - should use a different container
            nodes.erase(nodes.begin() + count);
            return true;
        }
        ++count;
    }

    LOGGER_ERROR("Parent with id of %llu does not exsist within the model graph", parentObj->getId());
    return false;
}

}
