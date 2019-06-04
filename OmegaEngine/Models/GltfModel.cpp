#include "GltfModel.h"
#include "Models/ModelMesh.h"
#include "Models/ModelTransform.h"
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

	void GltfModel::load(std::string filename)
	{
		// open the gltf file
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;

		std::string err, warn;
		std::string ext;

		FileUtil::GetFileExtension(filename, ext);
		bool ret = false;

		// gltf files can either be in binary or a human-readable format
		if (ext.compare("glb") == 0)
		{
			ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename.c_str());
		}
		else
		{
			ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename.c_str());
		}

		if (ret)
		{
			// first get all materials and textures associated with this model
			for (auto& tex : model.textures)
			{
				tinygltf::Image image = model.images[tex.source];
				textureManager.addGltfImage(image);
			}

			for (auto& sampler : model.samplers)
			{
				textureManager.addGltfSampler(set, sampler);
			}

			for (auto& mat : model.materials)
			{
				materialManager.addGltfMaterial(set, mat, textureManager);
			}
			textureManager.nextSet();

			// we are going to parse the node recursively to get all the info required for the space - this will add a new object per node - which are treated as models.
			// data will be passed to all the relevant managers for this object and components added automatically


			// skinning info
			componentInterface->getManager<TransformManager>().addGltfSkin(model, linearisedObjects);

			// animation
			animation_manager->addGltfAnimation(model, linearisedObjects);
		}
		else
		{
			LOGGER_ERROR("Error whilst parsing gltf file: %s", err.c_str());
		}
	}
	

	void GltfModel::parse(tinygltf::Model& model)
	{
		tinygltf::Scene &scene = model.scenes[model.defaultScene];;
		
		for (uint32_t i = 0; i < scene.nodes.size(); ++i)
		{
			auto& parentNode = std::make_unique<ModelNode>();
			extractNodeData(parentNode, model, model.nodes[scene.nodes[i]]);
			nodes.emplace_back(std::move(parentNode));
		}
	}

	void GltfModel::extractNodeData(std::unique_ptr<ModelNode>& node, tinygltf::Model& model, tinygltf::Node& gltfNode)
	{
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
				extractNodeData(node->children[node->children.size() - 1], model, model.nodes[gltfNode.children[i]]);
			}
		}

		// if the node has mesh data...
		if (gltfNode.mesh > -1)
		{
			node->mesh = std::make_unique<ModelMesh>();
			node->mesh->extractMeshData(model, gltfNode);
		}
	}
}
