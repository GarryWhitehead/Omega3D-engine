#include "RenderConfig.h"
#include "Utility/FileUtil.h"
#include "Utility/logger.h"

namespace OmegaEngine
{
	
	void RenderConfig::load()
	{
		std::string json;
		const char filename[] = "render_config.ini";		// probably need to check the current dir here
		if (!FileUtil::readFileIntoBuffer(filename, json))
		{
			return;
		}

		// if we cant parse the config, then go with the default values
		rapidjson::Document doc;
		if (doc.Parse(json.c_str()).HasParseError())
		{
			LOGGER_INFO("Unable to find render_config file. Using default settings...");
			return;
		}

		// general render settings
		if (doc.HasMember("Renderer"))
		{
			int renderer = doc["Renderer"].GetInt();
			general.renderer = static_cast<RendererType>(renderer);
		}

	}
}