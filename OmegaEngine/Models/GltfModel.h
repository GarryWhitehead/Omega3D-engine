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
			std::unique_ptr<ModelTransform> transform;
			std::unique_ptr<ModelMesh> mesh;
			std::unique_ptr<ModelSkin> skin;

			std::vector<std::unique_ptr<ModelNode> > children;
		};

		GltfModel();
		~GltfModel();

		void load(std::string filename);
		void parse(tinygltf::Model& model);
		void extractNodeData(std::unique_ptr<ModelNode>& node, tinygltf::Model& model, tinygltf::Node& gltfNode);

	private:

		std::vector<std::unique_ptr<ModelNode> > nodes;
	};

}