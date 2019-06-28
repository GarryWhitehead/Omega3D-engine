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
			assert(index < children.size());
			return children[index];
		}
			
		bool hasChildren() const
		{
			return !children.empty();
		}

		uint32_t childCount() const
		{
			return static_cast<uint32_t>(children.size());
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

		std::unique_ptr<ModelMesh> getMesh()
		{
			// Important: the caller takes ownership of the mesh data
			return std::move(mesh);
		}

		uint32_t getSkinIndex()
		{
			return skinIndex;
		}

		std::unique_ptr<ModelTransform> getTransform()
		{
			// Important: the caller takes ownership of the transform data
			return std::move(transform);
		}

		void setSkeletonRootFlag()
		{
			skeletonRoot = true;
		}

		void setJoint(int32_t index)
		{
			joint = index;
		}

		int32_t getJoint() const
		{
			return joint;
		}

		bool isSkeletonRoot() const
		{
			return skeletonRoot;
		}

		bool isJoint() const
		{
			return joint > -1;
		}

		bool hasAnimation() const
		{
			return !animChannelIndices.empty();
		}

		void setAnimationIndex(const uint32_t index, const uint32_t channelIndex)
		{
			animChannelIndices.emplace_back(channelIndex);
			animIndex = index;
		}

		uint32_t getAnimIndex() const
		{
			return animIndex;
		}

		std::vector<uint32_t> getChannelIndices() 
		{
			return animChannelIndices;
		}

	private:

		int32_t nodeIndex = -1;
		int32_t skinIndex = -1;

		std::unique_ptr<ModelTransform> transform;
		std::unique_ptr<ModelMesh> mesh;

		std::vector<std::unique_ptr<ModelNode> > children;

		// couple of flags regards skinning - points to the associated skin index
		bool skeletonRoot = false;
		int32_t joint = -1;


		// animation flag - indices required to link object with animation channel
		// this one node could have multiple channels assocaited with it.
		std::vector<uint32_t> animChannelIndices;
		int32_t animIndex = -1;
	};

}

