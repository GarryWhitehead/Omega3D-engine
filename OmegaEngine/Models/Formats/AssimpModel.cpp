#include "AssimpModel.h"

#include "utility/Logger.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/cimport.h"

namespace OmegaEngine
{

bool AssimpModel::load(Util::String filename)
{
	Assimp::Importer importer;
	
	const aiScene* scene = importer.ReadFile(filename.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs);
	if (!scene)
	{
		LOGGER_ERROR("Unable to open assimp model file %s.\n", filename.c_str());
		return false;
	}
}


}    // namespace OmegaEngine
