#pragma once
#include "XMLparse.h"
#include <vector>
#include <string>
#include "SiimpleCollada/ColladaSkeleton.h"
#include "SiimpleCollada/ColladaVertices.h"
#include "SiimpleCollada/ColladaMaterials.h"
#include "SiimpleCollada/ColladaAnimation.h"

class SimpleCollada
{
public:

	struct SceneData
	{
		ColladaVertices *meshData;		
		ColladaMaterials *materials;
		ColladaAnimation* animation;
		ColladaSkeleton *skeleton;

		// helper functions
		uint32_t numMeshes() const { return meshData->meshes.size(); }
	};

	SimpleCollada();
	~SimpleCollada();

	bool OpenColladaFile(std::string filename);
	SimpleCollada::SceneData* ImportColladaData();
	
private:

	XMLparse *m_xmlParse;

	SceneData *m_sceneData;
};



