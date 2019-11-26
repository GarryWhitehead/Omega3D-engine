#include "NodeInstance.h"

#include "Models/Formats/GltfModel.h"
#include "Models/SkinInstance.h"

#include "OEMaths/OEMaths_transform.h"

#include "utility/logger.h"

namespace OmegaEngine
{

NodeInstance::NodeInstance() noexcept
{
}

NodeInstance::~NodeInstance()
{
}

NodeInstance::NodeInfo* NodeInstance::findNode(Util::String id, NodeInfo* node)
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
}

NodeInstance::NodeInfo* NodeInstance::getNode(Util::String id)
{
   return findNode(id, rootNode);
}
    
bool NodeInstance::prepareNodeHierachy(cgltf_node* node, NodeInfo* parent, OEMaths::mat4f& parentTransform,
                                    GltfModel& model, size_t& nodeIdx)
{
	NodeInfo* newNode = new NodeInfo;
	newNode->parent = parent;
	newNode->id = nodeIdx++;

	if (node->mesh)
	{
		MeshInstance newMesh;
		newMesh.prepare(*node->mesh, model);
		newNode->hasMesh = true;

		if (node->skin)
		{
			// skins will be prepared later
			skins.emplace_back(node->skin);
		}

		// propogate transforms through node list
		prepareTranslation(node, newNode);
		newNode->localTransform = parentTransform * newNode->localTransform;
	}

	// now for the children of this node
	cgltf_node* const* childEnd = node->children + node->children_count;
	for (cgltf_node* const* child = node->children; child < childEnd; ++child)
	{
		if (!prepareNodeHierachy(node, newNode, newNode->nodeTransform, model, nodeIdx))
		{
			return false;
		}
	}

	return true;
}

void NodeInstance::prepareTranslation(cgltf_node* node, NodeInfo* newNode)
{
	// usually the gltf file will have a baked matrix or trs data
	if (node->has_matrix)
	{
        newNode->localTransform = OEMaths::mat4f(node->matrix);
	}
	else
	{
		if (node->has_translation)
		{
			newNode->translation = OEMaths::vec3f(node->translation);
		}
		if (node->has_rotation)
		{
			newNode->rotation = OEMaths::quatf(node->rotation);
		}
		if (node->has_scale)
		{
			newNode->scale = OEMaths::vec3f(node->scale);
		}

		newNode->nodeTransform =
		    OEMaths::mat4f::translate(newNode->translation) * newNode->rotation * OEMaths::mat4f::scale(newNode->scale);
	}
}

bool NodeInstance::prepare(cgltf_node* node, GltfModel& model)
{
	size_t nodeId = 0;
    if (!prepareNodeHierachy(node, nullptr, OEMaths::mat4f{}, model, nodeId))
	{
		return false;
	}
    
	return true;
}

}    // namespace OmegaEngine
