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

#include "NodeInstance.h"

#include "Formats/GltfModel.h"
#include "MeshInstance.h"
#include "NodeInstance.h"
#include "OEMaths/OEMaths_transform.h"
#include "SkinInstance.h"
#include "utility/Logger.h"

namespace OmegaEngine
{

NodeInfo::~NodeInfo()
{
    for (auto& child : children)
    {
        if (child)
        {
            delete child;
            child = nullptr;
        }
    }
}

NodeInfo::NodeInfo(const NodeInfo& rhs)
    : id(rhs.id)
    , skinIndex(rhs.skinIndex)
    , channelIndex(rhs.channelIndex)
    , hasMesh(rhs.hasMesh)
    , translation(rhs.translation)
    , scale(rhs.scale)
    , rotation(rhs.rotation)
    , localTransform(rhs.localTransform)
    , nodeTransform(rhs.nodeTransform)
{
    // if there already children, delete them
    if (!children.empty())
    {
        children.clear();
    }
    for (auto& child : rhs.children)
    {
        NodeInfo* newChild = new NodeInfo(*child);
        children.push_back(newChild);
        children.back()->parent = this;
    }
}

NodeInfo& NodeInfo::operator=(const NodeInfo& rhs)
{
    if (this != &rhs)
    {
        id = rhs.id;
        skinIndex = rhs.skinIndex;
        channelIndex = rhs.channelIndex;
        hasMesh = rhs.hasMesh;
        translation = rhs.translation;
        scale = rhs.scale;
        rotation = rhs.rotation;
        localTransform = rhs.localTransform;
        nodeTransform = rhs.nodeTransform;

        // if there already children, delete them
        if (!children.empty())
        {
            children.clear();
        }
        for (auto& child : rhs.children)
        {
            NodeInfo* newChild = new NodeInfo(*child);
            children.push_back(newChild);
            children.back()->parent = this;
        }
    }
    return *this;
}

// ================================================================================================================================

NodeInstance::NodeInstance()
{
}

NodeInstance::~NodeInstance()
{
    if (mesh)
    {
        delete mesh;
        mesh = nullptr;
    }
    if (skin)
    {
        delete skin;
        skin = nullptr;
    }
    if (rootNode)
    {
        delete rootNode;
        rootNode = nullptr;
    }
}

NodeInfo* NodeInstance::findNode(Util::String id, NodeInfo* node)
{
    NodeInfo* result = nullptr;
    if (node->id.compare(id))
    {
        return node;
    }
    for (NodeInfo* child : node->children)
    {
        result = findNode(id, child);
        if (result)
        {
            break;
        }
    }
    return result;
}

NodeInfo* NodeInstance::getNode(Util::String id)
{
    return findNode(id, rootNode);
}

bool NodeInstance::prepareNodeHierachy(
    cgltf_node* node,
    NodeInfo* newNode,
    NodeInfo* parent,
    OEMaths::mat4f& parentTransform,
    GltfModel& model,
    size_t& nodeIdx)
{
    assert(newNode);
    newNode->parent = parent;
    // newNode->id = nodeIdx++;

    if (node->mesh)
    {
        mesh = new MeshInstance();
        mesh->prepare(*node->mesh, model);
        newNode->hasMesh = true;

        if (node->skin)
        {
            skin = new SkinInstance();
            skin->prepare(*node->skin, *this);
        }

        // propogate transforms through node list
        prepareTranslation(node, newNode);
        newNode->localTransform = parentTransform * newNode->localTransform;
    }

    // now for the children of this node
    cgltf_node* const* childEnd = node->children + node->children_count;
    for (cgltf_node* const* child = node->children; child < childEnd; ++child)
    {
        NodeInfo* childNode = new NodeInfo();
        if (!prepareNodeHierachy(node, childNode, newNode, newNode->nodeTransform, model, nodeIdx))
        {
            return false;
        }
        newNode->children.emplace_back(childNode);
    }

    return true;
}

void NodeInstance::prepareTranslation(cgltf_node* node, NodeInfo* newNode)
{
    // usually the gltf file will have a baked matrix or trs data
    if (node->has_matrix)
    {
        newNode->localTransform = OEMaths::mat4f::makeMatrix(node->matrix);
    }
    else
    {
        if (node->has_translation)
        {
            newNode->translation =
                OEMaths::vec3f {node->translation[0], node->translation[1], node->translation[2]};
        }
        if (node->has_rotation)
        {
            newNode->rotation = OEMaths::quatf {
                node->rotation[0], node->rotation[1], node->rotation[2], node->rotation[3]};
        }
        if (node->has_scale)
        {
            newNode->scale = OEMaths::vec3f {node->scale[0], node->scale[1], node->scale[2]};
        }

        newNode->nodeTransform = OEMaths::mat4f::translate(newNode->translation) *
            newNode->rotation * OEMaths::mat4f::scale(newNode->scale);
    }
}

bool NodeInstance::prepare(cgltf_node* node, GltfModel& model)
{
    size_t nodeId = 0;
    OEMaths::mat4f transform;
    rootNode = new NodeInfo();

    if (!prepareNodeHierachy(node, rootNode, nullptr, transform, model, nodeId))
    {
        return false;
    }

    return true;
}

MeshInstance* NodeInstance::getMesh()
{
    assert(mesh);
    return mesh;
}

SkinInstance* NodeInstance::getSkin()
{
    if (!skin)
    {
        LOGGER_WARN(
            "This model contains no skinning data though you have tried to retrieve a skin?");
        return nullptr;
    }
    return skin;
}

NodeInfo* NodeInstance::getRootNode()
{
    assert(rootNode);
    return rootNode;
}

} // namespace OmegaEngine
