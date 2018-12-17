#pragma once

#include <string>
#include <fstream>

class FileManager
{
public:

	FileManager();
	~FileManager();

	bool readFileIntoBuffer(std::string filename, std::string& buffer);
};

