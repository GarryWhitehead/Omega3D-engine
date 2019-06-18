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

		void setJointFlag()
		{
			jointFlag = true;
		}

		bool isSkeletonRoot() const
		{
			return skeletonRoot;
		}

		bool isJoint() const
		{
			return jointFlag;
		}

		bool hasAnimation() const
		{
			return animChannelIndex > 0 && animIndex > 0;
		}

		void setAnimationIndex(const uint32_t index, const uint32_t channelIndex)
		{
			animChannelIndex = channelIndex;
			animIndex = index;
		}

		uint32_t getAnimIndex() const
		{
			return animIndex;
		}

		uint32_t getChannelIndex() const
		{
			return animChannelIndex;
		}

	private:

		int32_t nodeIndex = -1;
		int32_t skinIndex = -1;

		std::unique_ptr<ModelTransform> transform;
		std::unique_ptr<ModelMesh> mesh;

		std::vector<std::unique_ptr<ModelNode> > children;

		// couple of flags regards skinning 
		bool skeletonRoot = false;
		bool jointFlag = false;

		// animation flag - indices required to link object with animation channel
		int32_t animChannelIndex = -1;
		int32_t animIndex = -1;
	};

}

