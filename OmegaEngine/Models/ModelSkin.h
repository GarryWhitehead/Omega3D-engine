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

		void extractSkinData(tinygltf::Model& gltfModel, tinygltf::Skin& skin, GltfModel& model);

	private:

		std::string name;

		// these are both "got" from unique pointers. Naughty, probably better to chnage to indices at some point
		GltfModel::ModelNode* skeletonNode;
		std::vector<GltfModel::ModelNode*> joints;

		std::vector<OEMaths::mat4f> invBindMatrices;
		std::vector<OEMaths::mat4f> jointMatrices;
	};

}

