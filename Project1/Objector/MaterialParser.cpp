#include "MaterialParser.h"
#include <algorithm>
#include <sstream>
#include <iostream>

MaterialParser::MaterialParser(Objector::ModelInfo *model)
{
	m_currModel = model;
}


MaterialParser::~MaterialParser()
{
}

void MaterialParser::ImportMtlFile(std::string fileDir, std::string filename)
{
	
	std::fstream file;
	file.open(fileDir + filename, std::ios::in);
	if (!file.is_open()) {
		return;
	}
	std::cout << "Generating material data.......\n";
	uint32_t index = 0;
	std::string str;
	while (getline(file, str)) {

		// start by removing the comments from each line
		if (!objUtility.RemoveCommentsFromStr(str)) {
		
			// TODO: some .mtl files state the total number of materials so use this instead
			// start by finding the next new material designator
			std::stringstream ss(str);

			std::string type;
			ss >> type;

			if (type == "newmtl") {

				objMaterial mat;
				std::string name;
				ss >> name;
				mat.name = name;
				mat.index = index++;
				std::cout << "Processing material name: " << mat.name << "\n";
				name = this->ProcessNewMaterial(fileDir, mat, file);
				m_currModel->materials.push_back(mat);

				while (name != "end") {
					mat.name = name;
					mat.index = index++;
					std::cout << "Processing material name: " << mat.name << "\n";
					name = this->ProcessNewMaterial(fileDir, mat, file);
					m_currModel->materials.push_back(mat);
				}
			}
		}
	}
}

std::string MaterialParser::ProcessNewMaterial(std::string fileDir, objMaterial& mat, std::fstream& file)
{
	float x, y, z;
	std::string filename;
	std::string str;

	while (getline(file, str)) {
		if (!objUtility.RemoveCommentsFromStr(str)) {
		
			std::stringstream ss(str);

			std::string type;
			ss >> type;

			// if we have found the next material, return
			if (type == "newmtl") {
				std::string name;
				ss >> name;
				return name;
			}

			// texture maps
			// ambient texture map
			if (type == "map_Ka") {
				ss >> filename;
				mat.Map.ambient = fileDir + filename;
			}
			// diffuse texture map
			else if (type == "map_Kd") {
				ss >> filename;
				mat.Map.diffuse = fileDir + filename;
			}
			// diffuse specular map
			else if (type == "map_Ks") {
				ss >> filename;
				mat.Map.specular = fileDir + filename;
			}

			// check whether the string contains colour information
			// ambient colour
			else if (type == "Ka") {
				ss >> x; ss >> y; ss >> z;
				mat.Color.ambient = glm::vec3(x, y, z);
			}
			// diffuse colour
			else if (type == "Kd") {
				ss >> x; ss >> y; ss >> z;
				mat.Color.diffuse = glm::vec3(x, y, z);
			}
			// specular colour
			else if (type == "Ks") {
				ss >> x; ss >> y; ss >> z;
				mat.Color.specular = glm::vec3(x, y, z);
			}
				
			// TODO : add opacity, illum
		}
	}

	return "end";
}

std::string objMaterial::GetTextureTypeFilename(objTextureType type)
{
	std::string filename;

	switch (type) {
	case objTextureType::AMBIENT_TEXTURE:
		filename = this->Map.ambient;
		break;
	case objTextureType::DIFFUSE_TEXTURE:
		filename = this->Map.diffuse;
		break;
	case objTextureType::SPECULAR_TEXTURE:
		filename = this->Map.specular;
		break;
	case objTextureType::BUMP_TEXTURE:
		filename = this->Map.bump;
		break;
	}

	return filename;
}

bool objMaterial::hasTextureType(objTextureType type)
{
	bool hasType = true;

	switch (type) {
	case objTextureType::AMBIENT_TEXTURE:
		if (this->Map.ambient == "") {
			hasType = false;
		}
		break;
	case objTextureType::DIFFUSE_TEXTURE:
		if (this->Map.diffuse == "") {
			hasType = false;
		}
		break;
	case objTextureType::SPECULAR_TEXTURE:
		if (this->Map.specular == "") {
			hasType = false;
		}
		break;
	case objTextureType::BUMP_TEXTURE:
		if (this->Map.bump == "") {
			hasType = false;
		}
		break;
	}

	return hasType;
}