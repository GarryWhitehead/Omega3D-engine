#include "ModelSkin.h"
#include "Models/GltfModel.h"

#include <assert.h>
#include <memory>

namespace OmegaEngine
{
	
	ModelSkin::ModelSkin()
	{
	}


	ModelSkin::~ModelSkin()
	{
	}

	void ModelSkin::extractSkinData(tinygltf::Model& gltfModel, tinygltf::Skin& skin, std::unique_ptr<GltfModel::Model>& model, uint32_t skinIndex)
	{
		skin.name = skin.name.c_str();

		// Is this the skeleton root node?
		if (skin.skeleton > -1)
		{
			auto node = model->getNode(skin.skeleton);
			assert(node != nullptr);
			node->setSkeletonRootFlag();
		}

		// Does this skin have joint nodes?
		for (auto& jointIndex : skin.joints)
		{
			auto node = model->getNode(jointIndex);
			assert(node != nullptr);
			node->setJoint(skinIndex);
		}

		// get the inverse bind matricies, if there are any
		if (skin.inverseBindMatrices > -1)
		{
			tinygltf::Accessor accessor = gltfModel.accessors[skin.inverseBindMatrices];
			tinygltf::BufferView bufferView = gltfModel.bufferViews[accessor.bufferView];
			tinygltf::Buffer buffer = gltfModel.buffers[bufferView.buffer];

			invBindMatrices.resize(accessor.count);
			memcpy(invBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(OEMaths::mat4f));
		}
	}
}
