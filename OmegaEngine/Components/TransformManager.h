#pragma once

#include "Core/Scene.h"

#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"
#include "OEMaths/OEMaths_transform.h"

#include "Components/ComponentManager.h"

#include "Models/NodeInstance.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace OmegaEngine
{
// forward decleartions
class Object;
struct ModelSkin;

#define MAX_BONE_COUNT 256

struct TransformInfo
{
	NodeInstance::NodeInfo* root = nullptr;

	// the transform of this model - calculated by calling updateTransform()
	OEMaths::mat4f modelTransform;

	// the offset all skin indices will be adjusted by within this
	// node hierachy
	size_t skinOffset = 0;

	// skinning data - set by calling updateTransform()
	uint16_t jointCount = 0;
	std::array<OEMaths::mat4f, MAX_BONE_COUNT> jointMatrices;
};

class TransformManager : public ComponentManager
{

public:
	

	TransformManager();
	~TransformManager();

	/**
	* @brief Adds the node hierachy - reflects the layout obtained from 
	* wherever this originate from to make sure bone transforms are correct
	*/
	bool addNodeHierachy(NodeInstance& node, Object& obj, ModelSkin* skin, size_t skinCount);

	/**
	* @brief Creates a single node and adds the transform data to the root
	*/
	void addTransform(OEMaths::mat4f& loacl, OEMaths::vec3f& translation, OEMaths::vec3f& scale, OEMaths::quatf& rot);

	/**
	* @brief Updates the local matrix tree; returns the root node local matrix
	*/
	OEMaths::mat4f updateMatrix(NodeInstance::NodeInfo* node, OEMaths::mat4f& world);

	/**
	* @brief Called after a node heirachy is added or after a animation update, this function
	* works through the node structure combining the matrix of the child and their parent
	* starting from a mesh node.
	*/
	void updateModelTransform(NodeInstance::NodeInfo* parent, TransformInfo& transInfo);

	/**
	* @brief Updates the skeleton (node) hierachy of all objects associated with this component manager 
	* This should be called after any chnages - usually when updating animation transfroms
	*/
	void TransformManager::update();

	// object update functions
	void updateObjectTranslation(Object* obj, OEMaths::vec4f trans);
	void updateObjectScale(Object* obj, OEMaths::vec4f scale);
	void updateObjectRotation(Object* obj, OEMaths::quatf rot);

	// =================== getters ==========================
	TransformInfo& getTransform(const ObjHandle obj);

private:
	// transform data preserved in the node hierachal format
	// referenced by assoociated object
	std::vector<TransformInfo> nodes;

	// skinned data - inverse bind matrices and bone info
	std::vector<ModelSkin> skins;

	// store locally the aligned buffer sizes
	size_t transUboAlign = 0;
	size_t skinUboAlign = 0;

	// transform data for each object which will be added to the GPU
	TransformUbo* transformUbo = nullptr;
	SkinnedUbo* skinUbo = nullptr;

	size_t transUboCount = 0;
	size_t skinUboCount = 0;

	// flag which tells us whether we need to update the static data
	bool isDirty = true;
};

}    // namespace OmegaEngine
