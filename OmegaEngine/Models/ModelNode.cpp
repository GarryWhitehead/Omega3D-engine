#include "ModelNode.h"

#include "Models/Formats/GltfModel.h"

#include "OEMaths/OEMaths_transform.h"

#include "utility/logger.h"

namespace OmegaEngine
{
ModelNode::ModelNode()
{
}

ModelNode::~ModelNode()
{
}

bool ModelNode::prepareNodeHierachy(cgltf_node* node, NodeInfo* parent, OEMaths::mat4f& parentTransform,
                                    GltfModel& model, size_t& nodeIdx)
{
	NodeInfo* newNode = new NodeInfo;
	newNode->parent = parent;
	newNode->id = nodeIdx++;

	if (node->mesh)
	{
		ModelMesh mesh;
		mesh.prepare(*node->mesh, model);
		newNode->hasMesh = true;

		if (node->skin)
		{
			ModelSkin skin;
			skin.prepare(*node->skin);
			newNode->skinIndex = model.addSkin(skin);
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

void ModelNode::prepareTranslation(cgltf_node* node, NodeInfo* newNode)
{
	// usually the gltf file will have a baked matrix or trs data
	if (node->has_matrix)
	{
		newNode->localTransform = OEMaths::mat4f(node->matrix.data());
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

bool ModelNode::prepare(cgltf_node* node, GltfModel& model)
{
	size_t nodeId = 0;
	if (!prepareNodeHierachy(node, nullptr, OEMaths::mat4f{}, model, nodeId))
	{
		return false;
	}

	return true;
}

}    // namespace OmegaEngine