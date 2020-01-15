#pragma once

#include <fstream>
#include <string>
#include <vector>

namespace FileUtil
{

bool GetFileExtension(std::string filename, std::string& ext);
bool readFileIntoBuffer(std::string filename, std::string& buffer);

}; // namespace FileUtil
