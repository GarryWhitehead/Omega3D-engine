
#include "ModelGraph.h"

#include "Types/Object.h"
#include "utility/Logger.h"

namespace OmegaEngine
{

void ModelGraph::addNode(OEObject* parentObj, const OEMaths::mat4f& world)
{
    assert(parentObj);
    Node* node = new Node;
    node->parent = parentObj;
    node->world.worldMat = world;
    nodes.emplace_back(node);
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

    LOGGER_ERROR(
        "Parent with id of %llu does not exsist within the model graph", parentObj->getId());
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

        LOGGER_ERROR(
            "Chiild with id of %llu does not exsist within the model graph", childObj->getId());
        return false;
    }

    LOGGER_ERROR(
        "Parent with id of %llu does not exsist within the model graph", parentObj->getId());
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

    LOGGER_ERROR(
        "Parent with id of %llu does not exsist within the model graph", parentObj->getId());
    return false;
}

void ModelGraph::addWorldTransform(OEObject* parent, const OEMaths::mat4f& worldMat)
{
    for (auto* node : nodes)
    {
        if (*parent == *node->parent)
        {
            node->world.worldMat = worldMat;
        }
    }
}

void ModelGraph::addWorldTransform(
    OEObject* parent,
    const OEMaths::vec3f& scale,
    const OEMaths::vec3f& trans,
    const OEMaths::quatf& rot)
{
    for (auto* node : nodes)
    {
        if (*parent == *node->parent)
        {
            node->world.scale = scale;
            node->world.trans = trans;
            node->world.rot = rot;
            node->world.worldMat = OEMaths::mat4f::translate(trans) * rot * OEMaths::mat4f::scale(scale);
        }
    }
}

std::vector<ModelGraph::Node*>& ModelGraph::getNodeList()
{
    return nodes;
}

} // namespace OmegaEngine
