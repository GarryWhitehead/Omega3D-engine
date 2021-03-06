#pragma once

#include <fstream>
#include <string>

namespace OmegaEngine
{

namespace FileUtil
{
bool GetFileExtension(std::string filename, std::string &ext);
bool readFileIntoBuffer(std::string filename, std::string &buffer);

}; // namespace FileUtil

} // namespace OmegaEngine
