#include "Space.h"
#include "Utility/FileUtil.h"
#include "Utility/logger.h"
#include "Engine/Omega_Global.h"
#include "tiny_gltf.h"

namespace OmegaEngine
{

	Space::Space()
	{
	}


	Space::~Space()
	{
	}

	bool Space::loadModelData(std::string filename)
	{
		tinygltf::TinyGLTF loader;

		std::string err, warn;
		std::string ext;

		FileUtil::GetFileExtension(filename, ext);
		bool ret = false;
		if (ext.compare("glb") == 0) {
			ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename.c_str());
		}
		else {
			ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename.c_str());
		}

		// we can continue if we have just a warning for now
		if (!warn.empty()) {
			LOGGER_INFO("Problem whilst parsing gltf file: %s. Going to hope for the best...", warn);
		}

		// Although, if there's an error of any kind, time to quit
		if (!err.empty()) {
			LOGGER_ERROR("Error whilst parsing gltf file: %s", err);
			throw std::runtime_error("Unable to load scene data.");
		}
		if (!ret) {
			throw std::runtime_error("Unable to load scene data. No error message available.");
		}

		// everything seems ok, so now parse all the data we need...
		
	}
}
