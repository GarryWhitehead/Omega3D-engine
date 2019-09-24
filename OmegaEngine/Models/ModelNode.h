#pragma once

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

	ModelNode() = default;

	ModelNode();
	~ModelNode();

	bool prepare(cgltf_node* node, OEMaths::mat4f& parentTransform, GltfModel& model);
	void prepareTranslation(cgltf_node* node);

private:

	Util::String name;
	size_t skinIndex;

	// local decomposed node transfroms
	OEMaths::vec3f translation;
	OEMaths::vec3f scale = OEMaths::vec3f{ 1.0f };
	OEMaths::quatf rotation;

	// the transform matrix transformed by the parent matrix
	OEMaths::mat4f localTransform;

	// the transform matrix for this node
	OEMaths::mat4f nodeTransform;
};
} // namespace OmegaEngine
