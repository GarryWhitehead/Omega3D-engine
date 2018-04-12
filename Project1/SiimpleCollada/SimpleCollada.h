#pragma once
#include "XMLparse.h"
#include <vector>
#include <string>
#include <map>

enum class TransformType
{
	TRANSFORM_FLOAT,
	TRANSFORM_4X4
};

enum class MeshType
{
	MESH_TRIANGLE,
	MESH_POLYLIST
};

class SimpleCollada
{
public:

	struct InputSemanticInfo
	{
		std::string input;
		std::string output;
		std::string interpolation;
	};

	struct MeshInfo
	{
		std::string id;
		uint32_t count;
		uint8_t offset;
	};

	struct FaceInfo
	{
		std::string name;
		std::string material;
	};

	struct GeometryInfo 
	{
		MeshInfo vertex;
		MeshInfo normal;
		MeshInfo texcoord;
	};

	struct Face
	{
		MeshType type;
		std::vector<uint32_t> indices;
		FaceInfo info;

		uint32_t numIndices() { return indices.size(); }
	};

	struct Mesh
	{
		std::vector<glm::vec3> position;
		std::vector<glm::vec3> normal;
		std::vector<glm::vec3> texcoord;
		std::vector<glm::vec4> color;			// not checked for yet
		std::vector<Face> face;

		//helper functions
		uint32_t numPositions() const { return position.size(); }
		uint32_t numFaces() const { return face.size(); }
		bool hasColors() const { return !color.empty(); }
		uint32_t totalIndices();
	};

	struct LibraryAnimationFloatInfo
	{
		std::vector<float> time;
		std::vector<float> transform;
		std::vector<std::string> interpolation;
	};

	struct LibraryAnimation4X4Info
	{
		std::vector<float> time;
		std::vector<glm::mat4> transform;
		std::vector<std::string> interpolation;
	};

	struct MaterialInfo
	{
		std::string name;
		glm::vec4 ambient;
		glm::vec4 diffuse;
		glm::vec4 specular;
		glm::vec4 color;
		float shininess;
		float transparency;
	};

	struct WeightInfo
	{
		uint32_t vertexId;
		float weight;
	};

	struct TempWeightInfo
	{
		uint32_t boneIndex;
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

	struct SceneData
	{
		std::vector<Mesh> meshData;						// TODO : allow multiple meshes?
		TransformType transformType;
		std::vector<LibraryAnimationFloatInfo> libraryAnimationFloatData;
		std::vector<LibraryAnimation4X4Info> libraryAnimation4x4Data;
		std::vector<std::string> libraryImageData;
		std::vector<MaterialInfo> materialData;
		SkeletonTree skeletonData;
		SkinningInfo skinData;

		// helper functions
		uint32_t numMeshes() const { return meshData.size(); }
		uint32_t numMaterials() const { return materialData.size(); }

	};

	SimpleCollada();
	~SimpleCollada();

	bool OpenColladaFile(std::string filename);
	SimpleCollada::SceneData* ImportColladaData();
	void ImportLibraryAnimations();
	void ImportLibraryImages();
	void ImportGeometryData();
	void ImportLibraryMaterials();
	void ImportLibraryVisualScenes();
	void ImportLibraryControllers();
	void PrepareFinalData();
	uint32_t FindRootNode(std::string name, XMLPbuffer& buffer, uint32_t index);
	uint32_t FindNextBranch(XMLPbuffer& buffer, uint32_t& nodeIndex);
	bool FindNextNode(XMLPbuffer& buffer, uint32_t& nodeIndex);
	uint32_t ReadBranch(XMLPbuffer& buffer, uint32_t& nodeIndex, uint32_t parentId, uint32_t& childCount);
	void GetAnimationDataFloatTransform(XMLPbuffer &buffer);
	void GetAnimationData4X4Transform(XMLPbuffer &buffer);

	uint32_t GetFloatArrayInfo(std::string id, XMLPbuffer& buffer, uint32_t index, uint32_t semanticIndex);
	uint32_t FindBoneId(std::string id);

private:

	XMLparse *m_xmlParse;

	std::vector<InputSemanticInfo> m_semanticData;
	GeometryInfo m_geometryInfo;

	// used to store unique indices
	std::map<std::tuple<int, int, int>, uint32_t> m_indexRef;		// tuple - pos, uv, norm index
	uint32_t m_nextFreeIndex;			

	// temporary buffers to store vertex info, before creating unique indices data
	std::vector<glm::vec3> m_tempPosition;
	std::vector<glm::vec3> m_tempNormal;
	std::vector<glm::vec3> m_tempTexcoord;
	std::vector<std::vector<uint32_t> > m_tempIndices;
	std::map<uint32_t, std::vector<TempWeightInfo> > m_tempWeights;

	std::vector<uint32_t> m_vertCounts;
	std::vector<uint32_t> m_vertIndices;
	std::vector<float> m_weights;

	SceneData *m_sceneData;
	std::vector<std::string> m_nodeNames;
};



