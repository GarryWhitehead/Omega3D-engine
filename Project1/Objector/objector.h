#pragma once
#include "ObjUtility.h"
#include <string>
#include <vector>
#include <fstream>
#include <tuple>
#include <set>
#include <map>
#include "glm.hpp"

struct objMaterial;
class MaterialParser;

class Objector
{
public:

	struct FaceInfo
	{
		std::vector<uint32_t> indices;
	
		// helper functions
		uint32_t numIndices() const { return indices.size(); }
	};

	struct objMesh
	{
		std::string meshName;
		uint32_t materialIndex;

		std::vector<glm::vec3> posData;
		std::vector<glm::vec3> normData;
		std::vector<glm::vec2> uvData;
		std::vector<glm::vec3> freeFormData;
		std::vector<glm::vec3> colorData;
		std::vector<FaceInfo> faceData;

		// helper functions;
		bool hasPositions() const { return !posData.empty(); }
		bool hasNormals() const { return !normData.empty(); }
		bool hasUv() const { return !uvData.empty(); }
		bool hasColors() const { return !colorData.empty(); }

		uint32_t numPositions() const { return posData.size(); }
		uint32_t numNormals() const { return normData.size(); }
		uint32_t numUv() const { return uvData.size(); }
		uint32_t numFreeForm() const { return freeFormData.size(); }

		// face functions
		uint32_t numFaces() const { return faceData.size(); }
	};

	struct ModelInfo
	{
		ModelInfo() : 
			numMaterials(0),
			numMeshes(0)
		{}

		std::vector<objMesh> meshData;
		std::vector<objMaterial> materials;

		uint32_t numMaterials;
		uint32_t numMeshes;
	};

	Objector();
	~Objector();

	Objector(Objector&) = delete;
	Objector& operator=(Objector& obj) = delete;
	Objector& operator=(Objector&& obj) = delete;

	// core functions
	Objector::ModelInfo* ImportObjFile(std::string filename);
	void ReleaseObjFile();

	// helper functions
	bool isObjImported() const { return m_objFileImported; }
	
private:

	// private member functions
	void ProcessGroup(objMesh& mesh, std::fstream& file);
	void ProcessIndicies(std::string str, FaceInfo& face, objMesh& mesh);
	int32_t GetMaterialIndex(std::string mat);

	objMesh m_objInfo;

	MaterialParser *materialParser;
	ObjUtility objUtility;

	// temporary vectors for storing data pulled from file
	std::vector<glm::vec3> m_tempPos;
	std::vector<glm::vec2> m_tempUv;
	std::vector<glm::vec3> m_tempNorm;
	std::vector<glm::vec3> m_tempColor;
	std::map<std::tuple<int, int, int>, uint32_t> m_indexRef;		// tuple - pos, uv, norm index
	uint32_t m_nextIndex;

	ModelInfo *m_modelData;

	bool m_objFileImported;

};

