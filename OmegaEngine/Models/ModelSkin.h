#pragma once
#include "OEMaths/OEMaths.h"

#include "tiny_gltf.h"

namespace OmegaEngine
{
	// forward declerations
	class GltfModel;
	class ModelNode;

	class ModelSkin
	{

	public:

		ModelSkin();
		~ModelSkin();

		void extractSkinData(tinygltf::Model& gltfModel, tinygltf::Skin& skin, std::unique_ptr<GltfModel::Model>& model);

		OEMaths::mat4f* getInvBindData()
		{
			return invBindMatrices.data();
		}

		OEMaths::mat4f* getJointData()
		{
			return jointMatrices.data();
		}

		uint32_t getInvBindCount() const
		{
			return invBindMatrices.size();
		}

		uint32_t getJointCount() const
		{
			return jointMatrices.size();
		}

	private:

		std::string name;
		std::vector<OEMaths::mat4f> invBindMatrices;
		std::vector<OEMaths::mat4f> jointMatrices;
	};

}

