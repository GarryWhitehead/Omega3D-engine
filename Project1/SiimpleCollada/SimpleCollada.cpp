#include "SimpleCollada.h"
#include "SiimpleCollada/ColladaSkeleton.h"
#include "SiimpleCollada/ColladaVertices.h"
#include "SiimpleCollada/ColladaMaterials.h"
#include "SiimpleCollada/ColladaAnimation.h"

SimpleCollada::SimpleCollada() 
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
	
	// start with library animation data
	m_sceneData->animation = new ColladaAnimation(m_xmlParse);
	m_sceneData->animation->ImportLibraryAnimations();

	// skeleton data
	m_sceneData->skeleton= new ColladaSkeleton(m_xmlParse);
	m_sceneData->skeleton->ImportSkeleton();

	// vertex data
	m_sceneData->meshData = new ColladaVertices(m_xmlParse);
	m_sceneData->meshData->ImportVertices(m_sceneData->skeleton);

	// material data
	m_sceneData->materials = new ColladaMaterials(m_xmlParse);
	m_sceneData->materials->ImportMaterials();

	return m_sceneData;
}







