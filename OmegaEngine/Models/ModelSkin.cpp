#include "ModelSkin.h"

namespace OmegaEngine
{

	ModelSkin::ModelSkin()
	{
	}


	ModelSkin::~ModelSkin()
	{
	}

	void ModelSkin::extractSkinData(tinygltf::Model& model)
	{
		for (tinygltf::Skin& skin : model.skins)
		{
			skin.name = skin.name.c_str();

			// Is this the skeleton root node?
			if (skin.skeleton > -1)
			{
				assert(skin.skeleton < linearisedObjects.size());
				skinInfo.skeletonIndex = linearisedObjects[skin.skeleton];
				skinInfo.skeletonIndex.addComponent<SkinnedComponent>(static_cast<uint32_t>(skinBuffer.size() - 1));
			}

			// Does this skin have joint nodes?
			for (auto& jointIndex : skin.joints)
			{
				// we will check later if this node actually exsists
				assert(jointIndex < linearisedObjects.size() && jointIndex > -1);
				skinInfo.joints.push_back(linearisedObjects[jointIndex]);
			}

			// get the inverse bind matricies, if there are any
			if (skin.inverseBindMatrices > -1)
			{
				tinygltf::Accessor accessor = model.accessors[skin.inverseBindMatrices];
				tinygltf::BufferView bufferView = model.bufferViews[accessor.bufferView];
				tinygltf::Buffer buffer = model.buffers[bufferView.buffer];

				skinInfo.invBindMatrices.resize(accessor.count);
				memcpy(skinInfo.invBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(OEMaths::mat4f));
			}

			skinBuffer.push_back(skinInfo);
		}
	}
}
