#include "FileUtil.h"

namespace OmegaEngine
{

	namespace FileUtil
	{

		bool GetFileExtension(std::string filename, std::string& ext)
		{
			if (filename.empty()) {
				return false;
			}

			auto pos = filename.find_last_of('.');
			if (pos == std::string::npos) {
				return false;					// if there is no extension, then there's a formatting error somewhere
			}

			ext = filename.substr(pos + 1, filename.size());
			return true;
		}


		bool readFileIntoBuffer(std::string filename, std::string& buffer)
		{
			std::fstream file;
			file.open(filename, std::ios::in);
			if (!file.is_open) {
				return false;
			}

			std::string line;
			while (std::getline(file, line)) {
				line += "\n";
				buffer.append(line);
			}

			return true;
		}

	}
}

