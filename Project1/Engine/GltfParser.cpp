#include "GltfParser.h"

#include "DataTypes/TextureType.h"
#include "Utility/FileUtil.h"
#include "Utility/logger.h"

namespace OmegaEngine
{
	
	GltfParser::GltfParser()
	{
	}


	GltfParser::~GltfParser()
	{
	}

	bool GltfParser::parse(const char *filename)
	{
		

		// parse all the model data from the gltf file....
		loadNode();
		loadMaterial();
		loadTextures();
		loadAnimation();
		loadSkin();

		return true;
	}

	void GltfParser::loadNode()
	{
		
	}

	

	

	
	

	void GltfParser::loadLights()
	{
		// to add....
	}

	void GltfParser::loadEnvironment()
	{
		// to add extra.....

	}
}
