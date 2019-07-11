#pragma once
#include "Models/ModelAnimation.h"
#include "Models/ModelImage.h"
#include "Models/ModelMaterial.h"
#include "Models/ModelMesh.h"
#include "Models/Gltf/GltfNode.h"
#include "Models/ModelSkin.h"
#include "Models/ModelTransform.h"
#include "tiny_gltf.h"

#include <memory>
#include <vector>

namespace OmegaEngine
{

namespace GltfModel
{

struct Model
{
	std::vector<std::unique_ptr<ModelNode>> nodes;
	std::vector<std::unique_ptr<OmegaEngine::ModelMaterial>> materials;
	std::vector<std::unique_ptr<OmegaEngine::ModelImage>> images;
	std::vector<std::unique_ptr<OmegaEngine::ModelSkin>> skins;
	std::vector<std::unique_ptr<OmegaEngine::ModelAnimation>> animations;

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

std::unique_ptr<Model> load(std::string filename);

namespace Extract
{
std::unique_ptr<OmegaEngine::ModelImage> image(tinygltf::Model &model, tinygltf::Texture &texture);

std::unique_ptr<OmegaEngine::ModelMesh> mesh(tinygltf::Model &model, tinygltf::Node &node);

std::unique_ptr<OmegaEngine::ModelMaterial> material(tinygltf::Material &gltfMaterial);

std::unique_ptr<OmegaEngine::ModelAnimation> animation(tinygltf::Model &gltfModel,
                                                        tinygltf::Animation &anim,
                          std::unique_ptr<GltfModel::Model> &model, const uint32_t index);

std::unique_ptr<OmegaEngine::ModelTransform> transform(tinygltf::Node &node);

std::unique_ptr<OmegaEngine::ModelSkin> skin(tinygltf::Model &gltfModel, tinygltf::Skin &skin,
                     std::unique_ptr<GltfModel::Model> &model, uint32_t skinIndex);

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