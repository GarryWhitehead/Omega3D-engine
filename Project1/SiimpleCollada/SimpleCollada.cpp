#include "SimpleCollada.h"
#include <queue>


SimpleCollada::SimpleCollada() :
	m_nextFreeIndex(0)
{
}


SimpleCollada::~SimpleCollada()
{
}

bool SimpleCollada::OpenColladaFile(std::string filename)
{
	m_xmlParse = new XMLparse();
	m_xmlParse->LoadXMLDocument(filename);

	if (m_xmlParse->ReportErrors() == ErrorFlags::XMLP_ERROR_UNABLE_TO_OPEN_FILE) {

		return false;
	}

	return true;
}

SimpleCollada::SceneData* SimpleCollada::ImportColladaData()
{
	m_sceneData = new SceneData;
	
	m_sceneData->meshData.resize(1);			// may possibly allow for multiple meshes in the future if such files exsist

	// start with library animation data
	ImportLibraryAnimations();

	// Geomtry data
	ImportGeometryData();

	// material paths
	ImportLibraryImages();

	// material data
	ImportLibraryMaterials();

	// skeleton data
	ImportLibraryVisualScenes();

	// skinning data
	ImportLibraryControllers();

	// prepare the data for the user - condense indices into one uinqiue index for graphic API use and chnage bone weight vertex id accordingly
	PrepareFinalData();

	return m_sceneData;
}

void SimpleCollada::ImportLibraryAnimations()
{
	XMLPbuffer buffer;

	buffer = m_xmlParse->ReadTreeIntoBuffer("library_animations");
	if (m_xmlParse->ReportErrors() == ErrorFlags::XMLP_INCORRECT_FILE_FORMAT) {

		return;
	}

	// before stating, evaluate the type of transform used - X translation, thus only one float, or full transform, then using 4x4 matrix
	// This is found in the "technique_common" node in the child node param name or by checking the stride in accessor source
	uint32_t index = 0;
	index = m_xmlParse->FindElementInBuffer("technique_common", buffer, index);
	std::string type = m_xmlParse->ReadElementDataString("type", buffer, index + 2);		// making the assumpton that all data is laid out the same
	m_sceneData->transformType = (type == "float") ? TransformType::TRANSFORM_FLOAT : TransformType::TRANSFORM_4X4;

	index = 0;
	while (index < buffer.size()) {

		index = m_xmlParse->FindElementInBuffer("sampler id", buffer, index);
		if (index != UINT32_MAX) {

			// there are three types of input - INPUT which gives timing info; OUPUT - which gives the transform data at that particlaur time point
			// and INTERPOLATION - which gives the type of intrepolation to use between values (usually LINEAR)
			std::string  semantic;
			InputSemanticInfo semanticInfo;

			++index;
			for (int c = 0; c < 3; ++c) {

				semantic = m_xmlParse->ReadElementDataString("input semantic", buffer, index);
				if (!semantic.empty()) {

					if (semantic == "INPUT") {
						semanticInfo.input = m_xmlParse->ReadElementDataString("source", buffer, index++);
					}
					else if (semantic == "OUTPUT") {
						semanticInfo.output = m_xmlParse->ReadElementDataString("source", buffer, index++);
					}
					else if (semantic == "INTERPOLATION") {
						semanticInfo.interpolation = m_xmlParse->ReadElementDataString("source", buffer, index++);
					}
				}
			}
			m_semanticData.push_back(semanticInfo);
		}
	}

	if (m_sceneData->transformType == TransformType::TRANSFORM_4X4) {

		GetAnimationData4X4Transform(buffer);
	}
	else {
		GetAnimationDataFloatTransform(buffer);
	}

}

void SimpleCollada::GetAnimationDataFloatTransform(XMLPbuffer &buffer)
{
	// now we have the input semantic info - we can now read the corresponding data
	uint32_t index = 0;
	uint32_t semanticIndex = 0;

	while (index < buffer.size()) {
		
		LibraryAnimationFloatInfo animInfo;

		for (int c = 0; c < 3; ++c) {

			index = m_xmlParse->FindElementInBuffer("source id", buffer, index);
			if (index == UINT32_MAX) {
				return;
			}

			std::string id = m_xmlParse->ReadElementDataString("source id", buffer, index++);

			uint32_t arrayCount = 0;
			if (id == m_semanticData[semanticIndex].input) {

				arrayCount = GetFloatArrayInfo(m_semanticData[semanticIndex].input, buffer, index, semanticIndex);
				index = m_xmlParse->ReadElementArray<float>(buffer, animInfo.time, index, arrayCount);
			}
			else if (id == m_semanticData[semanticIndex].output) {

				arrayCount = GetFloatArrayInfo(m_semanticData[semanticIndex].output, buffer, index, semanticIndex);
				index = m_xmlParse->ReadElementArray<float>(buffer, animInfo.transform, index, arrayCount);
			}
		}

		m_sceneData->libraryAnimationFloatData.push_back(animInfo);
		++semanticIndex;
	}
}

void SimpleCollada::GetAnimationData4X4Transform(XMLPbuffer &buffer)
{
	// now we have the input semantic info - we can now read the corresponding data
	uint32_t index = 0;
	uint32_t semanticIndex = 0;

	LibraryAnimation4X4Info animInfo;

	while (index < buffer.size()) {

		for (int c = 0; c < 3; ++c) {

			index = m_xmlParse->FindElementInBuffer("source id", buffer, index);
			if (index == UINT32_MAX) {
				return;
			}

			std::string id = m_xmlParse->ReadElementDataString("source id", buffer, index++);

			uint32_t arrayCount = 0;
			if (id == m_semanticData[semanticIndex].input) {

				arrayCount = GetFloatArrayInfo(m_semanticData[semanticIndex].input, buffer, index, semanticIndex);
				index = m_xmlParse->ReadElementArray<float>(buffer, animInfo.time, index, arrayCount);
			}
			else if (id == m_semanticData[semanticIndex].output) {

				arrayCount = GetFloatArrayInfo(m_semanticData[semanticIndex].output, buffer, index, semanticIndex);
				index = m_xmlParse->ReadElementArrayMatrix(buffer, animInfo.transform, index, arrayCount);
			}
		}

		m_sceneData->libraryAnimation4x4Data.push_back(animInfo);
		++semanticIndex;
	}
}

uint32_t SimpleCollada::GetFloatArrayInfo(std::string id, XMLPbuffer& buffer, uint32_t index, uint32_t semanticIndex)
{
	// start by ensuring that this is a float array
	std::string arrayId = m_xmlParse->ReadElementDataString("float_array id", buffer, index);
	if (arrayId != id + "-array") {
		return 0;
	}

	uint32_t arrayCount = m_xmlParse->ReadElementDataInt("count", buffer, index);
	return arrayCount;
}

void SimpleCollada::ImportGeometryData()
{
	XMLPbuffer buffer;

	// begin by reading geomtry data into a buffer
	buffer = m_xmlParse->ReadTreeIntoBuffer("library_geometries");
	if (m_xmlParse->ReportErrors() == ErrorFlags::XMLP_INCORRECT_FILE_FORMAT) {

		return;
	}

	std::string semantic;
	uint32_t index = 0;

	// for some reason, some exporters interchange the word vertices and position for ids, so check whether this is the case
	index = m_xmlParse->FindElementInBuffer("vertices", buffer, index);
	++index;
	semantic = m_xmlParse->ReadElementDataString("semantic", buffer, index);
	if (semantic == "POSITION") {

		m_geometryInfo.vertex.id = m_xmlParse->ReadElementDataString("source", buffer, index);
	}

	// Get vertex information before importing data
	index = m_xmlParse->FindElementInBuffer("triangles", buffer, index++);

	// TODO - allow for multiple meshes
	for (int c = 0; c < 3; ++c) {

		semantic = m_xmlParse->ReadElementDataString("semantic", buffer, index);
		if (!semantic.empty()) {

			if (semantic == "VERTEX") {

				if (m_geometryInfo.vertex.id.empty()) {
					m_geometryInfo.vertex.id = m_xmlParse->ReadElementDataString("source", buffer, index);
				}
				m_geometryInfo.vertex.offset = m_xmlParse->ReadElementDataInt("offset", buffer, index++);
			}
			else if (semantic == "NORMAL") {
				m_geometryInfo.normal.id = m_xmlParse->ReadElementDataString("source", buffer, index);
				m_geometryInfo.normal.offset = m_xmlParse->ReadElementDataInt("offset", buffer, index++);
			}
			else if (semantic == "TEXCOORD") {
				m_geometryInfo.texcoord.id = m_xmlParse->ReadElementDataString("source", buffer, index);
				m_geometryInfo.texcoord.offset = m_xmlParse->ReadElementDataInt("offset", buffer, index++);
			}
		}
	}

	index = 0;
	uint32_t count = 0;
	// Read data into buffers - TODO : check counts and stride values of each data set

	for (int c = 0; c < 3; ++c) {

		index = m_xmlParse->FindElementInBuffer("source id", buffer, index);
		if (index == UINT32_MAX) {
			return;
		}
		++index;
		std::string sourceId = m_xmlParse->ReadElementDataString("id", buffer, index);
		if (sourceId == m_geometryInfo.vertex.id + "-array") {

			count = m_xmlParse->ReadElementDataInt("count", buffer, index);
			index = m_xmlParse->ReadElementArrayVec3(buffer, m_tempPosition, index, count);
			assert(!m_tempPosition.empty());
		}
		else if (sourceId == m_geometryInfo.normal.id + "-array") {

			count = m_xmlParse->ReadElementDataInt("count", buffer, index);
			index = m_xmlParse->ReadElementArrayVec3(buffer, m_tempNormal, index, count);
			assert(!m_tempNormal.empty());
		}
		else if (sourceId == m_geometryInfo.texcoord.id + "-array") {

			count = m_xmlParse->ReadElementDataInt("count", buffer, index);
			index = m_xmlParse->ReadElementArrayVec3(buffer, m_tempTexcoord, index, count);		// TODO:: texcoords can be found as either vec3 or vec2.Need to check the stride and parse as appropiate
			assert(!m_tempTexcoord.empty());
		}
	}
	
	// And finally the indices data - start by checking whether triangle mesh data is present
	index = 0;
	while (index < buffer.size()) {

		index = m_xmlParse->FindElementInBuffer("triangles", buffer, index);
		if (index == UINT32_MAX) {
			break;
		}

		Face face;
		face.type = MeshType::MESH_TRIANGLE;
		face.info.name = m_xmlParse->ReadElementDataString("material", buffer, index);
		uint32_t count = m_xmlParse->ReadElementDataInt("count", buffer, index);
		count *= 9;			// dealing with triangles - so 3 vertices per count * stride of 3

		index += 4;
		std::vector<uint32_t> indices;
		index = m_xmlParse->ReadElementArray<uint32_t>(buffer, indices, index, count);
		m_tempIndices.push_back(indices);

		m_sceneData->meshData[0].face.push_back(face);
	}

	// then check for polylists - these contain a vcount (number of indices to count) and p datasets
	index = 0;
	while (index < buffer.size()) {

		index = m_xmlParse->FindElementInBuffer("polylist", buffer, index);
		if (index == UINT32_MAX) {
			break;
		}

		Face face;
		face.type = MeshType::MESH_POLYLIST;
		face.info.name = m_xmlParse->ReadElementDataString("material", buffer, index);
		uint32_t count = m_xmlParse->ReadElementDataInt("count", buffer, index++);

		index = m_xmlParse->FindElementInBuffer("vcount", buffer, index);
		std::vector<uint32_t> vcount;
		index = m_xmlParse->ReadElementArray<uint32_t>(buffer, vcount, index, count);
		std::vector<uint32_t> indices;
		index = m_xmlParse->ReadElementArray<uint32_t>(buffer, indices, index, count);
		m_tempIndices.push_back(indices);

		m_sceneData->meshData[0].face.push_back(face);
	}
}



void SimpleCollada::ImportLibraryImages()
{
	m_sceneData->libraryImageData.clear();
	XMLPbuffer buffer;

	// begin by reading geomtry data into a buffer
	buffer = m_xmlParse->ReadTreeIntoBuffer("library_images");
	if (m_xmlParse->ReportErrors() == ErrorFlags::XMLP_INCORRECT_FILE_FORMAT) {

		return;
	}

	uint32_t index = 0;
	while (index < buffer.size()) {

		index = m_xmlParse->FindElementInBuffer("image", buffer, index);
		if (index == UINT32_MAX) {
			break;
		}
		std::string filename = m_xmlParse->ReadElementDataString("id", buffer, index++);
		m_sceneData->libraryImageData.push_back(filename);
	}
	
	if (m_sceneData->libraryImageData.empty()) {

		// let the user know that no image file was found
		
	}
}

void SimpleCollada::ImportLibraryMaterials()
{
	// first link materail id with face and get a count of the number of materials

	XMLPbuffer buffer;

	buffer = m_xmlParse->ReadTreeIntoBuffer("library_materials");
	if (m_xmlParse->ReportErrors() == ErrorFlags::XMLP_INCORRECT_FILE_FORMAT) {

		return;
	}

	uint32_t materialCount = 0;
	uint32_t index = 0;

	while (index < buffer.size()) {

		index = m_xmlParse->FindElementInBuffer("material", buffer, index);
		if (index == UINT32_MAX) {
			break;
		}
		std::string id = m_xmlParse->ReadElementDataString("id", buffer, index);
		std::string matName = m_xmlParse->ReadElementDataString("name", buffer, index++);

		for (auto& face : m_sceneData->meshData[0].face) {

			if (id == face.info.name) {
				face.info.material = matName;
				break;
			}
		}
		++materialCount;
	}

	// TODO: not using a lot of the information from the file. Will add when needed.
	// Now import the material data
	index = 0;
	XMLPbuffer effectsBuffer;
	effectsBuffer = m_xmlParse->ReadTreeIntoBuffer("library_effects");
	if (m_xmlParse->ReportErrors() == ErrorFlags::XMLP_INCORRECT_FILE_FORMAT) {

		return;
	}

	for(int c = 0; c < materialCount; ++c) {
	
		MaterialInfo material;

		index = m_xmlParse->FindElementInBuffer("effect", effectsBuffer, index);
		material.name = m_xmlParse->ReadElementDataString("name", effectsBuffer, index);
		index = m_xmlParse->FindElementInBuffer("ambient", effectsBuffer, index);
		material.ambient = m_xmlParse->ReadElementDataVec<glm::vec4>("color", effectsBuffer, ++index);
		index = m_xmlParse->FindElementInBuffer("diffuse", effectsBuffer, index);
		material.diffuse = m_xmlParse->ReadElementDataVec<glm::vec4>("color", effectsBuffer, ++index);
		index = m_xmlParse->FindElementInBuffer("specular", effectsBuffer, index);
		material.specular = m_xmlParse->ReadElementDataVec<glm::vec4>("color", effectsBuffer, ++index);
		index = m_xmlParse->FindElementInBuffer("shininess", effectsBuffer, index);
		material.shininess = m_xmlParse->ReadElementDataFloat("float", effectsBuffer, ++index);
		index = m_xmlParse->FindElementInBuffer("transparency", effectsBuffer, index);
		material.transparency = m_xmlParse->ReadElementDataFloat("float", effectsBuffer, ++index);
		m_sceneData->materialData.push_back(material);
	}
}

void SimpleCollada::ImportLibraryVisualScenes()
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
			
			for(auto& start : levelStartId) {

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

uint32_t SimpleCollada::FindNextBranch(XMLPbuffer& buffer, uint32_t& nodeIndex)
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

uint32_t SimpleCollada::ReadBranch(XMLPbuffer& buffer, uint32_t& nodeIndex, uint32_t parentId, uint32_t& childCount)
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
			m_sceneData->skeletonData.bones.push_back(node);
			++childCount;
			++nodeCount;
		}
	}

	if (parentId == 0) {
		--childCount;
		m_sceneData->skeletonData.bones[0].parentIndex = -1;
	}

	m_sceneData->skeletonData.bones[parentId].numChildren.push_back(childCount);
	m_sceneData->skeletonData.bones[parentId].childrenInd.push_back(m_sceneData->skeletonData.bones.size() - (childCount));

	return nodeCount;
}


uint32_t SimpleCollada::FindRootNode(std::string rootName, XMLPbuffer& buffer, uint32_t index)
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

bool SimpleCollada::FindNextNode(XMLPbuffer& buffer, uint32_t& nodeIndex)
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

void SimpleCollada::ImportLibraryControllers()
{
	XMLPbuffer buffer;

	buffer = m_xmlParse->ReadTreeIntoBuffer("library_controllers");
	if (m_xmlParse->ReportErrors() == ErrorFlags::XMLP_INCORRECT_FILE_FORMAT) {

		return;
	}

	uint32_t index = 0;

	index = m_xmlParse->FindElementInBuffer("joints", buffer, index);
	index += 2;
	std::string invBindSource = m_xmlParse->ReadElementDataString("source", buffer, index);

	index = m_xmlParse->FindElementInBuffer("vertex_weights", buffer, index);
	uint32_t vCount = m_xmlParse->ReadElementDataInt("count", buffer, index);
	++index;

	std::string jointSource, weightSource;

	for (int c = 0; c < 2; ++c) {
		std::string semantic = m_xmlParse->ReadElementDataString("semantic", buffer, index);

		if (semantic == "JOINT") {
			jointSource = m_xmlParse->ReadElementDataString("source", buffer, index++);
		}
		else if (semantic == "WEIGHT") {
			weightSource = m_xmlParse->ReadElementDataString("source", buffer, index++);
		}
	}

	// now import vcount (vertex weight count) and v (vertex weight indicies)
	index = m_xmlParse->ReadElementArray<uint32_t>(buffer, m_vertCounts, index, vCount);
	index = m_xmlParse->ReadElementArray<uint32_t>(buffer, m_vertIndices, index, vCount * 3);

	// import the joints, inverse binding and weights arrays
	index = 0;
	std::string id;
	uint32_t arrayCount = 0;

	for (int c = 0; c < 3; ++c) {
		
		index = m_xmlParse->FindElementInBuffer("source", buffer, index);
		id = m_xmlParse->ReadElementDataString("id", buffer, index++);
		if (id == jointSource) {

			arrayCount = m_xmlParse->ReadElementDataInt("count", buffer, index);
			index = m_xmlParse->ReadElementArray<std::string>(buffer, m_sceneData->skinData.joints, index, arrayCount);
		}
		else if (id == invBindSource) {

			arrayCount = m_xmlParse->ReadElementDataInt("count", buffer, index);
			index = m_xmlParse->ReadElementArrayMatrix(buffer, m_sceneData->skinData.invBind, index, arrayCount);
		}
		else if (id == weightSource) {

			arrayCount = m_xmlParse->ReadElementDataInt("count", buffer, index);
			index = m_xmlParse->ReadElementArray<float>(buffer, m_weights, index, arrayCount);
		}
	}

	// now sort the data so it is more ususally friendly. Add bone wights to corresponding bones and vertex Id
	uint32_t vertexId = 0;
	

	// load vertex bone data into a temporary buffer as the vertex Ids will be changing when we prepare the indices and vertices positions at the final stage
	uint32_t indexCount = 0;
	for(int c = 0; c < m_vertCounts.size(); ++c) {

		std::vector<TempWeightInfo> weightInfo;

		for (int i = 0; i < m_vertCounts[c]; ++i) {
			
			TempWeightInfo weight;
			assert(indexCount < m_vertIndices.size());
			weight.weight = m_weights[m_vertIndices[indexCount + 1]];
			std::string boneId = m_sceneData->skinData.joints[m_vertIndices[indexCount]];		// joint index - offset = 0

			size_t pos = boneId.find('<');				// check whther the boneID contains the XML element end designator - TODO: actually implement this check in the array reader
			if (pos != std::string::npos) {
				boneId = boneId.substr(0, pos - 1);
			}

			uint32_t boneIndex = FindBoneId(boneId);
			if (boneIndex == UINT32_MAX) {

				break;
			}

			weight.boneIndex = boneIndex;

			weightInfo.push_back(weight);
			indexCount += 2;					// stride of 2
			
		}

		m_tempWeights.insert(std::make_pair(vertexId, weightInfo));
		++vertexId;
	}
}

void SimpleCollada::PrepareFinalData()
{
	// Because openGL and Vulkan (and probably DirectX) only allow one set of indices and the data in the collada file is 
	// stored as two or three (assuming three for now) independent indices, we need to generate vertices for each unique set of indices
	// TODO : only dealing with <triangles>. also add support for polylists,etc.
	uint32_t faceIndex = 0;

	for (int i = 0; i < m_tempIndices.size(); ++i) {

		uint32_t posIndex = 0, uvIndex = 0, normIndex = 0;
		uint32_t index = 0;

		while(index < m_tempIndices[i].size()) {

			//for (int c = 0; c < 3; ++c) {			// we are assuming traiangles, so 3 vertices per set
				// an assumption is made that offet 0 = position; offset 1 = normal; offest 2 = texcoord
				// TODO: actually check the layout of the indices and check the stride of the texcoord data
				posIndex = m_tempIndices[i][index];
				normIndex = m_tempIndices[i][index + 1];
				uvIndex = m_tempIndices[i][index + 2];
				auto key = std::make_tuple(posIndex, normIndex, uvIndex);

				auto iter = m_indexRef.find(key);
				if (iter != m_indexRef.end()) {

					m_sceneData->meshData[0].face[faceIndex].indices.push_back(m_indexRef[key]);			// if key already exsists, no need to create a new vertex,
				}
				else {

					m_sceneData->meshData[0].position.push_back(m_tempPosition[posIndex]);		// if unique key, add new vertex data
					m_sceneData->meshData[0].normal.push_back(m_tempNormal[normIndex]);
					m_sceneData->meshData[0].texcoord.push_back(m_tempTexcoord[uvIndex]);

					m_sceneData->meshData[0].face[faceIndex].indices.push_back(m_nextFreeIndex);				// index this unique key in the indices buffer

					m_indexRef.insert(std::make_pair(key, m_nextFreeIndex));	// and add to index reference map 

					// generate the bone weight map using the newly derived vertex id
					for (auto& temp : m_tempWeights[posIndex]) {
						WeightInfo info;
						info.vertexId = m_nextFreeIndex;
						info.weight = temp.weight;
						m_sceneData->skeletonData.bones[temp.boneIndex].weights.push_back(info);
					}
					++m_nextFreeIndex;
				}
				index += 3;
				if (index >= m_tempIndices[i].size()) {
					break;
			//	}
			}
		}
		++faceIndex;
	}

	// free temp buffers to save memory
	m_tempIndices.clear();
	m_tempPosition.clear();
	m_tempNormal.clear();
	m_tempTexcoord.clear();
	m_tempWeights.clear();
	m_indexRef.clear();
}

uint32_t SimpleCollada::FindBoneId(std::string boneId)
{
	uint32_t index = 0;
	for (auto& bone : m_sceneData->skeletonData.bones) {

		if (bone.sid == boneId) {
			
			return index;
		}

		++index;
	}

	return UINT32_MAX;
}

uint32_t SimpleCollada::Mesh::totalIndices()
{
	uint32_t count = 0;
	for (auto &f : face) {
		count += f.numIndices();
	}
	return count;
}

