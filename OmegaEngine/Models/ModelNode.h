#pragma once

#include "tiny_gltf.h"

namespace OmegaEngine
{
	// forward declerations
	class ModelTransform;
	class ModelMesh;
	class ModelSkin;

	class ModelNode
	{

	public:

		ModelNode();
		~ModelNode();

		void extractNodeData(tinygltf::Model& model, tinygltf::Node& gltfNode, int32_t& index);

		ModelNode* getNodeRecursive(uint32_t index);

		std::unique_ptr<ModelNode>& getChildNode(const uint32_t index)
		{
			assert(index > children.size());
			return children[index];
		}
			
		bool hasChildren() const
		{
			return !children.empty();
		}

		uint32_t childCount() const
		{
			return children.size();
		}

		bool hasMesh() const
		{
			return mesh != nullptr;
		}

		bool hasSkin() const
		{
			return skinIndex != -1;
		}

		bool hasTransform() const
		{
			return transform != nullptr;
		}

		ModelMesh* getMesh()
		{
			return mesh.get();
		}

		uint32_t getSkinIndex()
		{
			return skinIndex;
		}

		ModelTransform* getTransform()
		{
			return transform.get();
		}

	private:

		int32_t nodeIndex = -1;
		int32_t skinIndex = -1;

		std::unique_ptr<ModelTransform> transform;
		std::unique_ptr<ModelMesh> mesh;

		std::vector<std::unique_ptr<ModelNode> > children;
	};

}

