#include "FileManager.h"



FileManager::FileManager()
{
}


FileManager::~FileManager()
{
}

bool FileManager::readFileIntoBuffer(std::string filename, std::string& buffer)
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
