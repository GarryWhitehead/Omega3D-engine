/* Copyright (c) 2018-2020 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

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
