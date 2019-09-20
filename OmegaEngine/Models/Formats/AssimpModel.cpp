#include "AssimpModel.h"

#include "utility/logger.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"

namespace OmegaEngine
{

bool AssimpModel::load(Util::String filename)
{
	Assimp::Importer importer;
	
	const aiScene* scene = importer.ReadFile(filename.c_str(), defaultFlags);
	if (!scene)
	{
		LOGGER_ERROR("Unable to open assimp model file %s.\n", filename.c_str());
		return false;
	}
}


}    // namespace OmegaEngine
