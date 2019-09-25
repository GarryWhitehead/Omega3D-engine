#pragma once
#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"
#include "OEMaths/OEMaths_transform.h"

#include "Utility/logger.h"

#include "Components/ComponentManager.h"

#include "Models/ModelNode.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace OmegaEngine
{
// forward decleartions
class Object;
struct TransformComponent;
struct SkinnedComponent;
struct ModelSkin;
class ModelTransform;

struct TransformInfo
{
	ModelNode::NodeInfo* root = nullptr;

	// the offset all skin indices will be adjusted by within this
	// node hierachy
	size_t skinOffset = 0;
};

class TransformManager : public ComponentManager
{

public:
	/// data that will be hosted on the gpu side
	struct TransformBufferInfo
	{
		OEMaths::mat4f modelMatrix;
	};

	struct SkinnedBufferInfo
	{
		OEMaths::mat4f jointMatrices[6];
		float jointCount;
	};

	// the number of models to allocate mem space for - this will need optimising
	// could also be dynamic and be altered to the archietecture being used
	const uint32_t TransformBlockSize = 25;
	const uint32_t SkinnedBlockSize = 25;

	TransformManager();
	~TransformManager();

	/**
	* @brief Adds the node hierachy - reflects the layout obtained from 
	* wherever this originate from to make sure bone transforms are correct
	*/
	bool addNodeHierachy(ModelNode& node, Object& obj, ModelSkin* skin, size_t skinCount);

	/**
	* @brief Creates a single node and adds the transform data to the root
	*/
	void addTransform(OEMaths::mat4f& loacl, OEMaths::vec3f& translation, OEMaths::vec3f& scale, OEMaths::quatf& rot);

	// update per frame
	void updateFrame();

	/**
	* @brief Updates the local matrix tree; returns the root node local matrix
	*/
	OEMaths::mat4f updateNodeLocalMatrix(ModelNode::NodeInfo* node, OEMaths::mat4f& world);

	void updateTransform();
	void updateTransformRecursive(Object& obj, uint32_t alignment, uint32_t skinnedAlignment);

	// object update functions
	void updateObjectTranslation(Object* obj, OEMaths::vec4f trans);
	void updateObjectScale(Object* obj, OEMaths::vec4f scale);
	void updateObjectRotation(Object* obj, OEMaths::quatf rot);

private:
	// transform data preserved in the node hierachal format
	// referenced by assoociated object
	std::vector<TransformInfo> nodes;

	// skinned data - inverse bind matrices and bone info
	std::vector<ModelSkin> skins;

	// store locally the aligned buffer sizes
	size_t transformAligned = 0;
	size_t skinnedAligned = 0;

	// transform data for each object which will be added to the GPU
	TransformBufferInfo* transformBufferData = nullptr;
	SkinnedBufferInfo* skinnedBufferData = nullptr;

	size_t transformBufferSize = 0;
	size_t skinnedBufferSize = 0;

	// flag which tells us whether we need to update the static data
	bool isDirty = true;
};

}    // namespace OmegaEngine
