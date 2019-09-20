#pragma once

#include "OEMaths/OEMaths.h"

#include "utility/String.h"

#include "cgltf/cgltf.h"

#include <memory>
#include <unordered_map>

namespace OmegaEngine
{

class ModelNode
{
public:

	ModelNode() = default;

	ModelNode();
	~ModelNode();

	bool prepare(cgltf_node* node, OEMaths::mat4f& parentTransform);
	void prepareTranslation(cgltf_node* node);

private:

	std::unique_ptr<ModelMesh> mesh;
	std::unique_ptr<ModelMaterial> material;
	std::unique_ptr<ModelSkin> skin;
	std::unique_ptr<ModelAnimation> animation;

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
