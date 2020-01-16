#include "FileUtil.h"

#include "Logger.h"

namespace FileUtil
{

bool GetFileExtension(std::string filename, std::string& ext)
{
    if (filename.empty())
    {
        return false;
    }

    auto pos = filename.find_last_of('.');
    if (pos == std::string::npos)
    {
        return false; // if there is no extension, then there's a formatting error somewhere
    }

    ext = filename.substr(pos + 1, filename.size());
    return true;
}

bool readFileIntoBuffer(std::string filename, std::string& buffer)
{
    std::ifstream file;
    file.open(filename);
    if (!file.is_open())
    {
        LOGGER_ERROR("Unable to open file: %s.", filename.c_str());
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        line += "\n";
        buffer.append(line);
    }

    return true;
}

bool readFileIntoBuffer(std::string filename, std::vector<std::string>& buffer)
{
    FILE* file = fopen(filename.c_str(), "r");
    if (!file)
    {
        LOGGER_ERROR("Unable to open file: %s.", filename.c_str());
        return false;
    }

    char line[128];
    while (fgets(line, 128, file))
    {
        buffer.emplace_back(std::string {line});
    }

    fclose(file);
    return true;
}


} // namespace FileUtil
