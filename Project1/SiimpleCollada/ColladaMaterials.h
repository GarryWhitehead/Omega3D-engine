#pragma once
#include "SiimpleCollada/XMLparse.h"

enum class MatType
{
	MAT_DIFFUSE,
	MAT_SPECULAR,
	MAT_NORMAL,
	MAT_NONE
};

class ColladaMaterials
{

public:

	struct MaterialInfo
	{
		std::string name;
		std::string filename;

		glm::vec4 ambient;
		glm::vec4 diffuse;
		glm::vec4 specular;
		glm::vec4 color;
		float shininess;
		float transparency;

		struct Texture
		{
			std::string diffuse;
			std::string specular;
			std::string normal;
		} texture;

		std::string GetMaterialType(MatType type) const;
		bool hasTexture(MatType type) const;
		bool hasTexture() const;
	};

	ColladaMaterials(XMLparse *parse);
	~ColladaMaterials();

	void ImportMaterials();
	void ImportLibraryMaterials();
	void ImportLibraryImages();

	std::vector<MaterialInfo> materials;

private:

	XMLparse *m_xmlParse;

	std::vector<std::string> libraryImageData;

};

