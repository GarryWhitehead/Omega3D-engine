#pragma once
#include "ObjUtility.h"
#include "objector.h"
#include <string>
#include <vector>
#include <fstream>
#include "glm.hpp"

enum class objTextureType
{
	DIFFUSE_TEXTURE,
	AMBIENT_TEXTURE,
	SPECULAR_TEXTURE,
	BUMP_TEXTURE
};

struct objMaterial
{
	objMaterial() : index(0) {}

	struct MatColor
	{
		glm::vec3 ambient;
		glm::vec3 diffuse;
		glm::vec3 specular;
		float specularExp;
	} Color;

	struct TexMap
	{
		std::string ambient;
		std::string diffuse;
		std::string specular;
		std::string bump;
		std::string alpha;
		std::string displacement;
		std::string highlight;
		std::string stencil;
	} Map;

	float opacity;
	uint32_t illumination;

	std::string name;
	uint32_t index;

	// helper functions
	std::string GetTextureTypeFilename(objTextureType type);
	bool hasTextureType(objTextureType type);
};

class MaterialParser
{
public:

	MaterialParser(Objector::ModelInfo *model);
	~MaterialParser();

	void ImportMtlFile(std::string fileDir, std::string filename);
	
private:

	std::string ProcessNewMaterial(std::string fileDir, objMaterial &mat, std::fstream& file);

	ObjUtility objUtility;

	Objector::ModelInfo *m_currModel;

	uint32_t flags;

};

