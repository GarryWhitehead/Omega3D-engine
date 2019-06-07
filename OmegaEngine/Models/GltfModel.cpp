#include "GltfModel.h"
#include "Models/ModelMesh.h"
#include "Models/ModelTransform.h"
#include "Models/ModelMaterial.h"
#include "Models/ModelImage.h"
#include "Models/ModelSkin.h"
#include "Models/ModelAnimation.h"
#include "Utility/FileUtil.h"
#include "Utility/logger.h"

namespace OmegaEngine
{

	GltfModel::GltfModel()
	{
	}


	GltfModel::~GltfModel()
	{
	}

	GltfModel::ModelNode* GltfModel::getNodeRecursive(std::unique_ptr<ModelNode>& node, uint32_t index)
	{
		ModelNode* foundNode;
		if (node->nodeIndex == index)
		{
			return node.get();
		}
		if (!node->children.empty())
		{
			for (auto& child : node->children)
			{
				foundNode = getNodeRecursive(child, index);
				if (foundNode)
				{
					break;
				}
			}
		}
		return foundNode;
	}

	GltfModel::ModelNode* GltfModel::getNode(uint32_t index)
	{
		ModelNode* foundNode = nullptr;
		for (auto& node : nodes)
		{
			foundNode = getNodeRecursive(node, index);
			if (foundNode)
			{
				break;
			}
		}
		return foundNode;
	}

	void GltfModel::load(std::string filename)
	{
		// open the gltf file
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;

		std::string err, warn;
		std::string ext;

		FileUtil::GetFileExtension(filename, ext);
		bool success = false;

		// gltf files can either be in binary or a human-readable format
		if (ext.compare("glb") == 0)
		{
			success = loader.LoadBinaryFromFile(&model, &err, &warn, filename.c_str());
		}
		else
		{
			success = loader.LoadASCIIFromFile(&model, &err, &warn, filename.c_str());
		}

		if (success)
		{
			parseNodes(model);

			// materials
			for (auto& material : model.materials)
			{
				auto& newMaterial = std::make_unique<ModelMaterial>();
				newMaterial->extractMaterialData(material);
				materials.emplace_back(std::move(newMaterial));
			}

			// images and samplers
			for (auto& texture : model.textures)
			{
				auto& newImage = std::make_unique<ModelImage>();
				newImage->extractfImageData(model, texture);
				images.emplace_back(std::move(newImage));
			}

			// skins
			for (tinygltf::Skin& skin : model.skins)
			{
				auto& newSkin = std::make_unique<ModelSkin>();
				newSkin->extractSkinData(model, skin, *this);
				skins.emplace_back(std::move(newSkin));
			}

			// animation
			for (tinygltf::Animation& anim : model.animations)
			{
				auto& newAnim = std::make_unique<ModelAnimation>();
				newAnim->extractAnimationData(model, anim, *this);
				animations.emplace_back(std::move(anim));
			}
		}
		else
		{
			LOGGER_ERROR("Error whilst parsing gltf file: %s", err.c_str());
		}
	}
	

	void GltfModel::parseNodes(tinygltf::Model& model)
	{
		tinygltf::Scene &scene = model.scenes[model.defaultScene];;

		for (uint32_t i = 0; i < scene.nodes.size(); ++i)
		{
			auto& parentNode = std::make_unique<ModelNode>();
			extractNodeData(parentNode, model, model.nodes[scene.nodes[i]], scene.nodes[i]);
			nodes.emplace_back(std::move(parentNode));
		}
	}

	void GltfModel::extractNodeData(std::unique_ptr<ModelNode>& node, tinygltf::Model& model, tinygltf::Node& gltfNode, int32_t& index)
	{
		node->nodeIndex = index;
		node->skinIndex = gltfNode.skin;

		// add all local and world transforms to the transform manager - also combines skinning info
		node->transform = std::make_unique<ModelTransform>();
		node->transform->extractTransformData(gltfNode);
		
		// if this node has children, recursively extract their info
		if (!gltfNode.children.empty())
		{
			for (uint32_t i = 0; i < gltfNode.children.size(); ++i)
			{
				auto& newNode = std::make_unique<ModelNode>();
				node->children.emplace_back(std::move(newNode));
				extractNodeData(node->children[node->children.size() - 1], model, model.nodes[gltfNode.children[i]], gltfNode.children[i]);
			}
		}

		// if the node has mesh data...
		if (gltfNode.mesh > -1)
		{
			// index is used to determine the correct nodes for applying joint transforms, etc.
			node->mesh = std::make_unique<ModelMesh>();
			node->mesh->extractMeshData(model, gltfNode);
		}
	}
}
