#include "GltfNode.h"
#include "Models/Gltf/GltfModel.h"

namespace OmegaEngine
{

namespace GltfModel
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
	transform = Extract::transform(gltfNode);

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
		mesh = Extract::mesh(model, gltfNode);

	}
}

} // namespace GltfModel
} // namespace OmegaEngine
