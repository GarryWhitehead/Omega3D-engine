#include "ModelNode.h"

#include "Models/ModelMesh.h"

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

bool ModelNode::prepare(cgltf_node* node, OEMaths::mat4f& parentTransform)
{
	// only deal with nodes that have meshes
	// transform nodes will be concenated
	if (node->mesh)
	{
		mesh->prepare(*node->mesh);

		if (node->mesh->primitives[0].material)
		{
			material->prepare(*node->mesh->primitives[0].material);
		}

		if (node->skin)
		{
			skin->prepare(*node->skin);
		}

		// propogate transforms through node list
		prepareTranslation(node);
		localTransform = parentTransform * localTransform;
	}

	// now for the children of this node
	cgltf_node* const* childEnd = node->children + node->children_count;
	for (cgltf_node* const* child = node->children; child < childEnd; ++child)
	{
		if (!prepare(node, nodeTransform))
		{
			return false;
		}
	}

	return true;
}

void ModelNode::prepareTranslation(cgltf_node* node)
{
	// usually the gltf file will have a baked matrix or trs data
	if (node->has_matrix)
	{
		localTransform = OEMaths::mat4f(node->matrix.data());
	}
	else
	{
		if (node->has_translation)
		{
			translation = OEMaths::vec3f(node->translation);
		}
		if (node->has_rotation)
		{
			rotation = OEMaths::quatf(node->rotation);
		}
		if (node->has_scale)
		{
			scale = OEMaths::vec3f(node->scale);
		}

		nodeTransform = OEMaths::mat4f::translate(translation) * rotation *
		                 OEMaths::mat4f::scale(scale);
	}
}

}    // namespace OmegaEngine