#pragma once

#include <fstream>
#include <string>
#include <vector>

namespace FileUtil
{

bool GetFileExtension(std::string filename, std::string& ext);
bool readFileIntoBuffer(std::string filename, std::string& buffer);
bool readFileIntoBuffer(std::string filename, std::vector<std::string>& buffer);

}; // namespace FileUtil
