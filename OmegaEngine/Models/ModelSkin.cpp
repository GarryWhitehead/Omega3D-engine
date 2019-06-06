#include "ModelSkin.h"
#include "Models/GltfModel.h"

#include <assert.h>

namespace OmegaEngine
{

	ModelSkin::ModelSkin()
	{
	}


	ModelSkin::~ModelSkin()
	{
	}

	void ModelSkin::extractSkinData(tinygltf::Model& gltfModel, GltfModel& model)
	{
		for (tinygltf::Skin& gltfSkin : gltfModel.skins)
		{
			Skin skin;
			skin.name = gltfSkin.name.c_str();

			// Is this the skeleton root node?
			if (gltfSkin.skeleton > -1)
			{
				skin.skeletonNode = model.getNode(gltfSkin.skeleton);
				assert(skin.skeletonNode != nullptr);
			}

			// Does this skin have joint nodes?
			for (auto& jointIndex : gltfSkin.joints)
			{
				auto node = model.getNode(jointIndex);
				assert(node != nullptr);
				skin.joints.emplace_back(node);
			}

			// get the inverse bind matricies, if there are any
			if (gltfSkin.inverseBindMatrices > -1)
			{
				tinygltf::Accessor accessor = gltfModel.accessors[gltfSkin.inverseBindMatrices];
				tinygltf::BufferView bufferView = gltfModel.bufferViews[accessor.bufferView];
				tinygltf::Buffer buffer = gltfModel.buffers[bufferView.buffer];

				skin.invBindMatrices.resize(accessor.count);
				memcpy(skin.invBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(OEMaths::mat4f));
			}

			skins.emplace_back(std::move(skin));
		}
	}
}
