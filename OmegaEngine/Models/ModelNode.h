#pragma once

#include "Models/ModelMesh.h"

#include "OEMaths/OEMaths.h"

#include "utility/String.h"

#include "cgltf/cgltf.h"

#include <memory>
#include <unordered_map>

namespace OmegaEngine
{

// forward decleartions
class GltfModel;

class ModelNode
{
public:
	/**
	* @brief Keep the node heirachy obtained from the gltf file as this
	* will be needed for bone transforms
	*/
	struct NodeInfo
	{
		NodeInfo()
		{
		}
		~NodeInfo()
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

		// no copying allowed
		NodeInfo(const NodeInfo&) = delete;
		NodeInfo& operator=(const NodeInfo&) = delete;
		NodeInfo(NodeInfo&&) = default;
		NodeInfo& operator=(NodeInfo&&) = default;

		// The id of the node is derived from the index - and is used to locate
		// this node if it's a joint or animation target
		size_t id;

		// the index of the skin associated with this node
		size_t skinIndex = -1;

		// a flag indicating whether this node contains a mesh
		// the mesh is actaully stored outside the hierachy
		bool hasMesh = false;

		// local decomposed node transfroms
		OEMaths::vec3f translation;
		OEMaths::vec3f scale = OEMaths::vec3f{ 1.0f };
		OEMaths::quatf rotation;

		// the transform matrix transformed by the parent matrix
		OEMaths::mat4f localTransform;

		// the transform matrix for this node = T*R*S
		OEMaths::mat4f nodeTransform;

		// parent of this node. Null signifies the root
		NodeInfo* parent = nullptr;

		// children of this node
		std::vector<NodeInfo*> children;
	};

	ModelNode() = default;

	ModelNode();
	~ModelNode();

	bool prepareNodeHierachy(cgltf_node* node, NodeInfo* parent, OEMaths::mat4f& parentTransform, GltfModel& model, size_t& nodeIdx);
	void prepareTranslation(cgltf_node* node, NodeInfo* newNode);
	bool prepare(cgltf_node* node, GltfModel& model);

	friend class Scene;
	friend class TransformManager;

private:
	// we expect one mesh per node hierachy!
	ModelMesh mesh;

	// the node hierachy
	NodeInfo* rootNode = nullptr;
};
}    // namespace OmegaEngine
