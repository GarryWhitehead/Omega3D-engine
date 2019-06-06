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

		struct Skin
		{
			std::string name;

			// these are both "got" from unique pointers. Naughty, probably better to chnage to indices at some point
			GltfModel::ModelNode* skeletonNode;
			std::vector<GltfModel::ModelNode*> joints;

			std::vector<OEMaths::mat4f> invBindMatrices;
			std::vector<OEMaths::mat4f> jointMatrices;
		};

		ModelSkin();
		~ModelSkin();

		void extractSkinData(tinygltf::Model& gltfModel, GltfModel& model);

	private:

		std::vector<Skin> skins;
	};

}

