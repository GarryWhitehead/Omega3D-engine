#pragma once
#include "SiimpleCollada/XMLparse.h"

class ColladaSkeleton
{
public:

	struct WeightInfo
	{
		uint32_t vertexId;
		float weight;
	};

	struct Node
	{
		Node() :
			numChildren(0)
		{}

		std::string name;
		std::string sid;
		glm::mat4 transform;
		int32_t parentIndex;
		std::vector<uint32_t> childrenInd;
		std::vector<uint32_t> numChildren;

		std::vector<WeightInfo> weights;

		//helper functions
		uint32_t numWeights() const { return weights.size(); }
	};

	struct SkeletonTree
	{
		std::vector<Node> bones;

		//helper functions
		glm::mat4 GetLocalTransform(std::string name)
		{
			for (uint32_t c = 0; c < bones.size(); ++c) {

				if (bones[c].sid == name) {

					return bones[c].transform;
				}
			}
		}

	} skeletonData;

	ColladaSkeleton(XMLparse *parse);
	~ColladaSkeleton();

	void ImportSkeleton();
	void ImportLibraryVisualScenes();
	uint32_t FindBoneId(std::string id);

private:

	XMLparse *m_xmlParse;

	uint32_t FindRootNode(std::string name, XMLPbuffer& buffer, uint32_t index);
	uint32_t FindNextBranch(XMLPbuffer& buffer, uint32_t& nodeIndex);
	bool FindNextNode(XMLPbuffer& buffer, uint32_t& nodeIndex);
	uint32_t ReadBranch(XMLPbuffer& buffer, uint32_t& nodeIndex, uint32_t parentId, uint32_t& childCount);

	std::vector<std::string> m_nodeNames;
};

