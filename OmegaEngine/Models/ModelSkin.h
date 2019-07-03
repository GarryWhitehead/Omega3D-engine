#pragma once
#include "OEMaths/OEMaths.h"

#include "tiny_gltf.h"

#include <memory>

namespace OmegaEngine
{
// forward declerations
class ModelNode;
namespace GltfModel
{
struct Model;
}

class ModelSkin
{

public:
	ModelSkin();
	~ModelSkin();

	void extractSkinData(tinygltf::Model &gltfModel, tinygltf::Skin &skin,
	                     std::unique_ptr<GltfModel::Model> &model, uint32_t skinIndex);

	OEMaths::mat4f *getInvBindData()
	{
		return invBindMatrices.data();
	}

	OEMaths::mat4f *getJointData()
	{
		return jointMatrices.data();
	}

	uint32_t getInvBindCount() const
	{
		return static_cast<uint32_t>(invBindMatrices.size());
	}

	uint32_t getJointCount() const
	{
		return static_cast<uint32_t>(jointMatrices.size());
	}

private:
	std::string name;
	std::vector<OEMaths::mat4f> invBindMatrices;
	std::vector<OEMaths::mat4f> jointMatrices;
};

} // namespace OmegaEngine
