#include "ColladaSkeleton.h"

ColladaSkeleton::ColladaSkeleton(XMLparse *parse) :
	m_xmlParse(parse)
{
}


ColladaSkeleton::~ColladaSkeleton()
{
}

void ColladaSkeleton::ImportSkeleton()
{
	ImportLibraryVisualScenes();
}

void ColladaSkeleton::ImportLibraryVisualScenes()
{
	XMLPbuffer buffer;

	buffer = m_xmlParse->ReadTreeIntoBuffer("library_visual_scenes");
	if (m_xmlParse->ReportErrors() == ErrorFlags::XMLP_INCORRECT_FILE_FORMAT) {

		return;
	}

	// joint data can be exported as multiple skeletons which is found in <instance_controller>
	// So, get a list of all the nodes present within the skeleton data
	uint32_t index = 0;
	index = m_xmlParse->FindElementInBuffer("instance_controller", buffer, index);
	++index;

	while (m_xmlParse->CheckElement("skeleton", buffer, index)) {

		std::string nodeName = m_xmlParse->ReadElementDataString("skeleton", buffer, index++);
		m_nodeNames.push_back(nodeName);
	}

	std::vector<uint32_t> levelStartId{ 0 };
	std::vector<uint32_t> bufferOffset{ 0 };
	uint32_t level = 0;
	int32_t id = 0;
	uint32_t parentId = 0;

	uint32_t nodeIndex = FindRootNode(m_nodeNames[0], buffer, 0);		// find the root node

	while (nodeIndex < buffer.size()) {

		uint32_t childCount = 0;
		uint32_t size = ReadBranch(buffer, nodeIndex, parentId, childCount);
		id += size;

		uint32_t nodesClosed = FindNextBranch(buffer, nodeIndex);
		uint32_t offset = id - nodesClosed;
		id -= nodesClosed;

		if (id < 0) {
			break;
		}
		else if (offset < levelStartId[level]) {

			for (auto& start : levelStartId) {

				if (offset < start) {

					--level;
					levelStartId.pop_back();
					bufferOffset.pop_back();
				}
			}
			parentId = id + bufferOffset[level];
		}
		else {
			parentId = id + bufferOffset[level];
			level++;
			levelStartId.push_back(size + levelStartId[level - 1]);
			bufferOffset.push_back(levelStartId[level - 1] + childCount);
		}
	}
}

uint32_t ColladaSkeleton::FindNextBranch(XMLPbuffer& buffer, uint32_t& nodeIndex)
{
	uint32_t closeCount = 0;
	while (nodeIndex < buffer.size()) {

		std::string line = buffer[nodeIndex];
		size_t pos = line.find("node");
		if (pos != std::string::npos) {

			pos = line.find("<node");
			if (pos != std::string::npos) {
				return closeCount;
			}

			pos = line.find("/node");
			if (pos != std::string::npos) {
				++closeCount;
			}
		}
		++nodeIndex;
	}
}

uint32_t ColladaSkeleton::ReadBranch(XMLPbuffer& buffer, uint32_t& nodeIndex, uint32_t parentId, uint32_t& childCount)
{
	uint32_t nodeCount = 0;

	while (m_xmlParse->CheckElement("node", buffer, nodeIndex)) {

		std::string type = m_xmlParse->ReadElementDataString("type", buffer, nodeIndex);
		if (type == "NODE") {
			nodeIndex += 2;
			++nodeCount;
		}
		else if (type == "JOINT") {

			Node node;
			node.name = m_xmlParse->ReadElementDataString("name", buffer, nodeIndex);
			node.sid = m_xmlParse->ReadElementDataString("sid", buffer, nodeIndex++);
			node.transform = m_xmlParse->ReadElementDataMatrix("matrix", buffer, nodeIndex++);
			node.parentIndex = parentId;
			skeletonData.bones.push_back(node);
			++childCount;
			++nodeCount;
		}
	}

	if (parentId == 0) {
		--childCount;
		skeletonData.bones[0].parentIndex = -1;
	}

	skeletonData.bones[parentId].numChildren.push_back(childCount);
	skeletonData.bones[parentId].childrenInd.push_back(skeletonData.bones.size() - (childCount));

	return nodeCount;
}


uint32_t ColladaSkeleton::FindRootNode(std::string rootName, XMLPbuffer& buffer, uint32_t index)
{
	uint32_t rootIndex = index;
	while (rootIndex < buffer.size()) {
		rootIndex = m_xmlParse->FindElementInBuffer("node", buffer, rootIndex);
		std::string name = m_xmlParse->ReadElementDataString("id", buffer, rootIndex);
		if (name == rootName) {
			return rootIndex;
		}
		++rootIndex;
	}
}

bool ColladaSkeleton::FindNextNode(XMLPbuffer& buffer, uint32_t& nodeIndex)
{
	while (nodeIndex < buffer.size()) {

		std::string line = buffer[nodeIndex];
		size_t pos = line.find("<node");
		if (pos != std::string::npos) {
			return true;
		}
		++nodeIndex;
	}
	return false;
}

uint32_t ColladaSkeleton::FindBoneId(std::string boneId)
{
	uint32_t index = 0;
	for (auto& bone : skeletonData.bones) {

		if (bone.sid == boneId) {

			return index;
		}

		++index;
	}

	return UINT32_MAX;
}