#include "objector.h"
#include "MaterialParser.h"
#include <algorithm>
#include <sstream>
#include <iostream>

Objector::Objector() :
	m_objFileImported(false)
{
	m_tempPos.clear();
	m_tempUv.clear();
	m_tempNorm.clear();
}


Objector::~Objector()
{
	ReleaseObjFile();
}

Objector::ModelInfo* Objector::ImportObjFile(std::string filename)
{	
	// Open file
	std::fstream file;
	file.open(filename, std::ios::in);
	if (!file.is_open()) {
		return nullptr;
	}
	
	m_modelData = new Objector::ModelInfo();
	materialParser = new MaterialParser(m_modelData);

	m_nextIndex = 0;

	std::string fileDir = filename.substr(0, filename.find_last_of('/') + 1);

	std::string str;
	while (getline(file, str)) {
		
		// start by removing the comments from each line
		if (!objUtility.RemoveCommentsFromStr(str)) {

			std::stringstream ss(str);
			std::string type;
			ss >> type;

			// check whether a material file is specified
			if (type == "mtllib") {

				std::string filename;
				ss >> filename;
				materialParser->ImportMtlFile(fileDir, filename);
				m_modelData->numMaterials = m_modelData->materials.size();
			}

		
			// 'g' denotes a group object in .obj format
			else if (type == "g" || type == "o") {

				objMesh mesh;
				std::string name;
				ss >> name;
				mesh.meshName = name;
				std::cout << "Group " << mesh.meshName << ", Processing Data......\n";

				// process the groupd data into the relevant containers
				this->ProcessGroup(mesh, file);
				m_modelData->meshData.push_back(mesh);
			}
			//else {

				// not all .obj files are designated by groups, so check whether there is an appropiate form and begin 
				// processing data if so
				//if (objUtility.CheckStringForForm(str)) {
					
				//	std::cout << "No group identifier. Processing data anyway....\n";
				//	objMesh mesh;
				//	this->ProcessGroup(mesh, file);
				//	m_modelData.meshData.push_back(mesh);
				//}
			//}
		}
	}

	m_modelData->numMeshes = m_modelData->meshData.size();
	
	m_objFileImported = true;
	return m_modelData;
}

void Objector::ProcessGroup(objMesh& mesh, std::fstream& file)
{
	std::string str;
	
	while (!file.eof()) {

		int oldPos = file.tellg();

		// now sort all the vert, norm and indicies into their relevant containers
		while (getline(file, str)) {

			if (!objUtility.RemoveCommentsFromStr(str)) {

				std::stringstream ss(str);
				std::string type;
				ss >> type;

				// if we have found another object or group, return
				if (type == "o" || type == "g") {
					file.seekg(oldPos);
					return;
				}

				// check if the materail to use is specified
				if (type == "usemtl") {

					std::string mat;
					ss >> mat;
					mesh.materialIndex = GetMaterialIndex(mat);
				}

				// or conatins normal data (vn)
				else if (type == "vn") {
					glm::vec3  norm;
					ss >> norm.x;
					ss >> norm.y;
					ss >> norm.z;
					m_tempNorm.push_back(norm);
					//std::cout << "norm data - xyz: " << norm.x << ", " << norm.y << ", " << norm.z << "\n";
				}

				// or conatins texture uv (vt)
				else if (type == "vt") {
					glm::vec2  uv;
					ss >> uv.x;
					ss >> uv.y;
					m_tempUv.push_back(uv);
					//std::cout << "uv data - uv: " << uv.x << ", " << uv.y << "\n";
				}

				// or conatins normal data (vp)
				else if (type == "vp") {
					glm::vec3  geo;
					ss >> geo.x;
					ss >> geo.y;
					ss >> geo.z;
					mesh.freeFormData.push_back(geo);
				}

				// or conatins vertex data (vn)
				else if (type == "v") {
					glm::vec3  vert;
					ss >> vert.x;
					ss >> vert.y;
					ss >> vert.z;
					m_tempPos.push_back(vert);
					//std::cout << "vert data - xyz: " << vert.x << ", " << vert.y << ", " << vert.z << "\n";

					// certain .obj files have colour information stored at the end of the position data. So, check whether this is the end of the string
					if (!ss.eof()) {

						glm::vec3 color;
						ss >> color.r;
						ss >> color.g;
						ss >> color.b;
						m_tempColor.push_back(color);
					}
				}

				// or conatins indices (face) data (f)
				else if (type == "f") {

					FaceInfo face;
					this->ProcessIndicies(str, face, mesh);
					mesh.faceData.push_back(face);
				}
			}
		}
	}
}

void Objector::ProcessIndicies(std::string str, FaceInfo& face, objMesh& mesh)
{
	uint32_t faceIndex = 0;
	std::string strVal;
	int posIndex = 0, uvIndex = 0, normIndex = 0;

	int n = 0;
	// find start of data
	while (!isdigit(str[n])) {
		++n;
	}

	// .obj face indices can either be in the form f v1, v2, v3...., v1 / vt1 /vn1.... or f v1 // vn1
	for (uint32_t c = n; c < str.size(); ++c) {
		if (isdigit(str[c])) {
			strVal = strVal + str[c];							// if its a number, add to our temp string

			if (!isdigit(str[c + 1])) {					// peek ahead and if the next char isn't a number convert to number and store
				int value = std::stoi(strVal);
				
				strVal.clear();

				if (faceIndex == 0) {					// no '/' - only contain vertex indicies
					posIndex = value - 1;
				}
				else if (faceIndex == 1) {				// one '/' - contains tex indicies - though skipped in the case of v // vn
					uvIndex = value - 1;
				}
				else if (faceIndex == 2) {				// conatins v / vt /vn
					normIndex = value - 1;
				}
			}
		}
		else if (str[c] == '/') {						
			++faceIndex;
		}

		if (str[c] == ' ' || c == str.size() - 1) {		// if we have reached white space then it is the end of this particlar set of indicies
			
			auto key = std::make_tuple(posIndex, uvIndex, normIndex);
			auto iter = m_indexRef.find(key);
			if (iter != m_indexRef.end()) {

				face.indices.push_back(m_indexRef[key]);			// if key already exsists, no need to create a new vertex,
			}
			else {

				mesh.posData.push_back(m_tempPos[posIndex]);		// if unique key, add new vertex data
				mesh.normData.push_back(m_tempNorm[normIndex]);

				if (!m_tempColor.empty()) {
					mesh.colorData.push_back(m_tempColor[posIndex]);	// the colour data follows the same pattern as position
				}
				if (!m_tempUv.empty()) {							// double check that this file actually conatins uv information
					mesh.uvData.push_back(m_tempUv[uvIndex]);
				}

				face.indices.push_back(m_nextIndex);				// index this unique key in the indices buffer

				m_indexRef.insert(std::make_pair(key, m_nextIndex));	// and add to index reference map 
				++m_nextIndex;
			}

			faceIndex = 0;
		}
	}
}

int32_t Objector::GetMaterialIndex(std::string matName)
{
	int32_t index = -1;
	for (auto& material : m_modelData->materials) {
		
		if (material.name == matName) {
			index = material.index;
		}
	}

	return index;
}

void Objector::ReleaseObjFile()
{
	if (m_modelData != nullptr) {
		
		delete m_modelData;
	}
	m_modelData = nullptr;
}