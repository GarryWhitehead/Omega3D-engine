#pragma once
#include "Models/ModelAnimation.h"
#include "Models/ModelImage.h"
#include "Models/ModelMaterial.h"
#include "Models/ModelMesh.h"
#include "Models/Gltf/GltfNode.h"
#include "Models/ModelSkin.h"
#include "Models/ModelTransform.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"

#include <memory>
#include <vector>

namespace OmegaEngine
{

class Model
{
	
public:

	Model load(std::string filename);

private:
	
	ModelNode nodes;
	ModelMaterial materials;
	ModelImage images;
	ModelSkin skins;
	ModelAnimation animations;

	ModelNode *getNode(uint32_t index)
	{
		ModelNode *foundNode = nullptr;
		for (auto &node : nodes)
		{
			foundNode = node->getNodeRecursive(index);
			if (foundNode)
			{
				break;
			}
		}
		return foundNode;
	}
};



template <typename T>
void parseIndices(tinygltf::Accessor accessor, tinygltf::BufferView bufferView,
                  tinygltf::Buffer buffer, std::vector<uint32_t> &indiciesBuffer,
                  uint32_t indexStart)
{
	T *buf = new T[accessor.count];
	memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
	       accessor.count * sizeof(T));

	// copy the data to our indices buffer at the correct offset
	for (uint32_t j = 0; j < accessor.count; ++j)
	{
		indiciesBuffer.push_back(buf[j] + indexStart);
	}

	delete buf;
}
} // namespace Extract
} // namespace GltfModel

} // namespace OmegaEngine