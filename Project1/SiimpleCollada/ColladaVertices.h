#pragma once
#include <vector>
#include "SiimpleCollada/XMLparse.h"
#include "SiimpleCollada/ColladaSKeleton.h"
#include <map>


enum class MeshType
{
	MESH_TRIANGLE,
	MESH_POLYLIST
};

class ColladaVertices
{
public:

	struct FaceInfo
	{
		MeshType type;
		std::string material;
		std::vector<uint32_t> indices;	

		uint32_t numIndices() const { return indices.size(); }
	};

	struct MeshInfo
	{
		std::string id;
		uint32_t count;
		uint8_t offset;
	};

	struct GeometryInfo
	{
		MeshInfo vertex;
		MeshInfo normal;
		MeshInfo texcoord;
	};

	
	struct Mesh
	{
		std::vector<glm::vec3> position;
		std::vector<glm::vec3> normal;
		std::vector<glm::vec2> texcoord;
		std::vector<glm::vec4> color;			// not checked for yet
		std::vector<FaceInfo> face;

		//helper functions
		uint32_t numPositions() const { return position.size(); }
		uint32_t numFaces() const { return face.size(); }
		bool hasColors() const { return !color.empty(); }
	};

	struct TempWeightInfo
	{
		uint32_t boneIndex;
		float weight;
	};

	struct SkinningInfo
	{
		glm::mat4 bindShape;
		std::vector<glm::mat4> invBind;
		std::vector<std::string> joints;

		// helper functions
		uint32_t FindBone(std::string name)
		{
			for (uint32_t c = 0; c < joints.size(); ++c) {
				if (joints[c] == name) {
					return c;
				}
			}
			return 0;
		}
	};

	ColladaVertices(XMLparse *parse);
	~ColladaVertices();

	void ImportVertices(ColladaSkeleton *p_skeleton);
	
	// main vertices buffers open to the public
	std::vector<Mesh> meshes;
	std::vector<SkinningInfo> skinData;

private:

	uint32_t ProcessGeometryData(XMLPbuffer& buffer, uint32_t index, MeshType type, uint32_t& meshIndex);
	uint32_t ProcessControllers(XMLPbuffer& buffer, uint32_t index, ColladaSkeleton *p_skeleton);
	void ProcessVertices(ColladaSkeleton::SkeletonTree& skeleton, uint32_t meshIndex);
	void ClearTempBuffers();

	XMLparse *m_xmlParse;
	GeometryInfo m_geometryInfo;

	//bone weights related stuff
	std::vector<float> m_weights;

	// temporary buffers to store vertex info, before creating unique indices data
	std::vector<glm::vec3> m_tempPosition;
	std::vector<glm::vec3> m_tempNormal;
	std::vector<glm::vec2> m_tempTexcoord;
	std::vector<std::vector<uint32_t> > m_tempIndices;
	std::map<uint32_t, std::vector<TempWeightInfo> > m_tempWeights;		// temp weights for each mesh
	
	// used to store unique indices
	std::map<std::tuple<int, int, int>, uint32_t> m_indexRef;		// tuple - pos, uv, norm index
	uint32_t m_nextFreeIndex;

	bool m_generateFace;
	bool useSetTwoForUv;
};

