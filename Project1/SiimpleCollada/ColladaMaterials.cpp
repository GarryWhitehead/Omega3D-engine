#include "ColladaMaterials.h"
#include "SiimpleCollada/XMLparse.h"
#include "SiimpleCollada/SimpleCollada.h"


ColladaMaterials::ColladaMaterials(XMLparse *parse) :
	m_xmlParse(parse)
{
}


ColladaMaterials::~ColladaMaterials()
{
}

void ColladaMaterials::ImportMaterials()
{
	ImportLibraryMaterials();
	ImportLibraryImages();
}

void ColladaMaterials::ImportLibraryImages()
{
	libraryImageData.clear();
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
		libraryImageData.push_back(filename);
	}

	if (libraryImageData.empty()) {

		// let the user know that no image file was found

	}
}

void ColladaMaterials::ImportLibraryMaterials()
{
	// first link materail id with face and get a count of the number of materials

	XMLPbuffer buffer;

	buffer = m_xmlParse->ReadTreeIntoBuffer("library_materials");
	if (m_xmlParse->ReportErrors() == ErrorFlags::XMLP_INCORRECT_FILE_FORMAT) {

		return;
	}

	// TODO: not using a lot of the information from the file. Will add when needed.
	// Now import the material data
	uint32_t index = 0;
	XMLPbuffer effectsBuffer;
	effectsBuffer = m_xmlParse->ReadTreeIntoBuffer("library_effects");
	if (m_xmlParse->ReportErrors() == ErrorFlags::XMLP_INCORRECT_FILE_FORMAT) {

		return;
	}

	while (index < effectsBuffer.size()) {

		MaterialInfo material;

		uint32_t startIndex = m_xmlParse->FindElementInBuffer("effect", effectsBuffer, index);
		if (startIndex == UINT32_MAX) {
			break;
		}
		material.name = m_xmlParse->ReadElementDataString("id", effectsBuffer, startIndex);

		index = m_xmlParse->FindChildElementInBuffer("effect", "ambient", effectsBuffer, startIndex);
		if (index != UINT32_MAX) {
			material.ambient = m_xmlParse->ReadElementDataVec<glm::vec4>("color", effectsBuffer, index + 1);
		}
		
		index = m_xmlParse->FindChildElementInBuffer("effect", "diffuse", effectsBuffer, startIndex);
		if (index != UINT32_MAX) {
			if (m_xmlParse->CheckElement("texture", effectsBuffer, index + 1)) {
				material.texture.diffuse = m_xmlParse->ReadElementDataString("texture", effectsBuffer, index + 1);
			}
			else {
				material.diffuse = m_xmlParse->ReadElementDataVec<glm::vec4>("color", effectsBuffer, index + 1);
			}
		}

		index = m_xmlParse->FindChildElementInBuffer("effect", "normal", effectsBuffer, startIndex);
		if (index != UINT32_MAX) {
			if (m_xmlParse->CheckElement("texture", effectsBuffer, index + 1)) {
				material.texture.normal = m_xmlParse->ReadElementDataString("texture", effectsBuffer, index + 1);
			}
		}
	
		index = m_xmlParse->FindChildElementInBuffer("effect", "specular", effectsBuffer, startIndex);
		if (index != UINT32_MAX) {
			material.specular = m_xmlParse->ReadElementDataVec<glm::vec4>("color", effectsBuffer, index + 1);
		}
		else {
			material.specular = glm::vec4(1.0f);
		}

		index = m_xmlParse->FindChildElementInBuffer("effect", "shininess", effectsBuffer, startIndex);
		if (index != UINT32_MAX) {
			material.shininess = m_xmlParse->ReadElementDataFloat("float", effectsBuffer, index + 1);
		}
		else {
			material.shininess = 0.0f;
		}

		index = m_xmlParse->FindChildElementInBuffer("effect", "transparency", effectsBuffer, startIndex);
		if (index != UINT32_MAX) {
			material.transparency = m_xmlParse->ReadElementDataFloat("float", effectsBuffer, index + 1);
		}
		else {
			material.transparency = 0.0f;
		}

		materials.push_back(material);
		index = startIndex + 1;
	}

	// now chnage the name of used in library effects to the material name used by the vertices
	index = 0;

	for (auto& mat : materials) {

		index = m_xmlParse->FindElementInBuffer("material", buffer, index);
		std::string url = m_xmlParse->ReadElementDataString("url", buffer, index + 1);

		if (url == mat.name) {
			mat.name = m_xmlParse->ReadElementDataString("id", buffer, index);
		}
		++index;
	}
	
}

std::string ColladaMaterials::MaterialInfo::GetMaterialType(MatType type) const
{
	std::string filename;
	switch (type) {
	case MatType::MAT_DIFFUSE:
		filename = texture.diffuse;
		break;
	case MatType::MAT_NORMAL:
		filename = texture.normal;
		break;
	}
	return filename;
}

bool ColladaMaterials::MaterialInfo::hasTexture(MatType type) const
{
	switch (type) {
	case MatType::MAT_DIFFUSE:
		return !texture.diffuse.empty();
		break;
	case MatType::MAT_NORMAL:
		return !texture.normal.empty();
		break;
	}
}

bool ColladaMaterials::MaterialInfo::hasTexture() const
{
	// all materials must have a diffuse texture
	if (texture.diffuse.empty()) {

		return false;
	}

	return true;
}