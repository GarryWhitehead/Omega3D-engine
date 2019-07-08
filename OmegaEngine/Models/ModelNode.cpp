#include "ModelNode.h"
#include "Models/ModelMesh.h"
#include "Models/ModelSkin.h"
#include "Models/ModelTransform.h"

namespace OmegaEngine
{

ModelNode::ModelNode()
{
}

ModelNode::~ModelNode()
{
}

ModelNode *ModelNode::getNodeRecursive(uint32_t index)
{
	ModelNode *foundNode = nullptr;
	if (nodeIndex == index)
	{
		return this;
	}
	if (!children.empty())
	{
		for (auto &child : children)
		{
			foundNode = child->getNodeRecursive(index);
			if (foundNode)
			{
				break;
			}
		}
	}
	return foundNode;
}

void ModelNode::extractNodeData(tinygltf::Model &model, tinygltf::Node &gltfNode, int32_t &index)
{
	nodeIndex = index;
	skinIndex = gltfNode.skin;

	// add all local and world transforms to the transform manager - also combines skinning info
	transform = std::make_unique<ModelTransform>();
	transform->extractTransformData(gltfNode);

	// if this node has children, recursively extract their info
	if (!gltfNode.children.empty())
	{
		for (uint32_t i = 0; i < gltfNode.children.size(); ++i)
		{
			auto &newNode = std::make_unique<ModelNode>();
			auto newNodePtr = newNode.get();
			children.emplace_back(std::move(newNode));
			newNodePtr->extractNodeData(model, model.nodes[gltfNode.children[i]],
			                            gltfNode.children[i]);
		}
	}

	// if the node has mesh data...
	if (gltfNode.mesh > -1)
	{
		// index is used to determine the correct nodes for applying joint transforms, etc.
		mesh = std::make_unique<ModelMesh>();
		mesh->extractGltfMeshData(model, gltfNode);
	}
}
} // namespace OmegaEngine
