/* Copyright (c) 2018-2020 Garry Whitehead
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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
