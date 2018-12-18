#pragma once

#include <string>
#include <fstream>

namespace OmegaEngine
{

	namespace FileUtil
	{
		bool GetFileExtension(std::string filename, std::string& ext);
		bool readFileIntoBuffer(std::string filename, std::string& buffer);

	};

}

