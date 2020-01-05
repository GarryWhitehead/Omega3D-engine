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
class SkinInstance;

struct TransformInfo
{
	NodeInstance::NodeInfo* root = nullptr;

	// the transform of this model - calculated by calling updateTransform()
	OEMaths::mat4f modelTransform;

	// the offset all skin indices will be adjusted by within this
	// node hierachy. We use this also to signify if this model has a skin
	uint32_t skinOffset = UINT32_MAX;

	// skinning data - set by calling updateTransform()
	std::vector<OEMaths::mat4f> jointMatrices;
};

class TransformManager : public ComponentManager
{

public:
	
    static constexpr uint32_t MAX_BONE_COUNT = 256;
    
	TransformManager();
	~TransformManager();

	/**
	* @brief Adds the node hierachy - reflects the layout obtained from 
	* wherever this originate from to make sure bone transforms are correct
	*/
	bool addNodeHierachy(NodeInstance& node, Object& obj, SkinInstance* skin);

	/**
	* @brief Creates a single node and adds the transform data to the root
	*/
	void addTransform(OEMaths::mat4f& loacl, OEMaths::vec3f& translation, OEMaths::vec3f& scale, OEMaths::quatf& rot);

	/**
	* @brief Updates the local matrix tree; returns the root node local matrix
	*/
	OEMaths::mat4f updateMatrix(NodeInstance::NodeInfo* node);

	/**
	* @brief Called after a node heirachy is added or after a animation update, this function
	* works through the node structure combining the matrix of the child and their parent
	* starting from a mesh node.
	*/
	void updateModelTransform(NodeInstance::NodeInfo* parent, TransformInfo& transInfo);

	void updateModel(Object& obj);

	// object update functions
	void updateObjectTranslation(Object& obj, const OEMaths::vec3f& trans);
	void updateObjectScale(Object& obj, const OEMaths::vec3f& scale);
	void updateObjectRotation(Object& obj, const OEMaths::quatf& rot);

	// =================== getters ==========================
	TransformInfo& getTransform(const ObjHandle obj);

private:

	// transform data preserved in the node hierachal format
	// referenced by assoociated object
	std::vector<TransformInfo> nodes;

	// skinned data - inverse bind matrices and bone info
	std::vector<SkinInstance> skins;

};

}    // namespace OmegaEngine
