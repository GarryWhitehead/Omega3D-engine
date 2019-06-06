#pragma once

#include "tiny_gltf.h"

namespace OmegaEngine
{
	// forward declerations
	class ModelTransform;
	class ModelMesh;
	class ModelSkin;

	class GltfModel
	{

	public:

		struct ModelNode
		{
			int32_t nodeIndex = -1;
			int32_t skinIndex = -1;

			std::unique_ptr<ModelTransform> transform;
			std::unique_ptr<ModelMesh> mesh;
			std::unique_ptr<ModelSkin> skin;

			std::vector<std::unique_ptr<ModelNode> > children;
		};

		GltfModel();
		~GltfModel();

		void load(std::string filename);
		void parseNodes(tinygltf::Model& model);
		void extractNodeData(std::unique_ptr<ModelNode>& node, tinygltf::Model& model, tinygltf::Node& gltfNode, int32_t& index);
		
		ModelNode* getNode(uint32_t index);
		
	private:

		ModelNode* getNodeRecursive(std::unique_ptr<ModelNode>& node, uint32_t index);

		std::vector<std::unique_ptr<ModelNode> > nodes;
	};

}