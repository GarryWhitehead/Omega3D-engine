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
		NodeInfo() = default;
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

		Util::String name;
		size_t skinIndex;

		// local decomposed node transfroms
		OEMaths::vec3f translation;
		OEMaths::vec3f scale = OEMaths::vec3f{ 1.0f };
		OEMaths::quatf rotation;

		// the transform matrix transformed by the parent matrix
		OEMaths::mat4f localTransform;

		// the transform matrix for this node = T*R*S
		OEMaths::mat4f nodeTransform;

		// parent of this node
		NodeInfo* parent = nullptr;

		// children of this node
		std::vector<NodeInfo*> children;
	};

	ModelNode() = default;

	ModelNode();
	~ModelNode();

	bool prepare(cgltf_node* node, OEMaths::mat4f& parentTransform, GltfModel& model);
	void prepareTranslation(cgltf_node* node);

	friend class Scene;
	friend class TransformManager;

private:
	// we expect one mesh per node hierachy!
	ModelMesh mesh;

	// the node hierachy
	NodeInfo* rootNode = nullptr;
};
}    // namespace OmegaEngine
