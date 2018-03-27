#include "serialisation.h"
#include <sstream>

Archiver::Archiver()
{
}

void Archiver::OpenFile(const std::string filename, SaveMode saveMode, FileMode fileMode)
{
	std::ios_base::openmode flags;
	
	if (fileMode == FileMode::FILE_IN) {
		flags = std::fstream::in;
	}
	else {
		flags = std::fstream::out;
	}

	if (saveMode == SaveMode::SAVE_BINARY) {
		flags |= std::fstream::binary;
	}

	file.open(filename, flags);

	if (!file.is_open()) {
		*g_filelog << "Critical error!! Unable to open" << filename << " during serialisation. Exiting......\n";
		exit(EXIT_FAILURE);
	}
}

// custom variable type functions
void Archiver::Serialise(char& value, const var_info& info)
{
	std::string line;
	SerialiseText<int32_t>(value, info, line);
	if (!line.empty())
		value = line[0];
}

void Archiver::Serialise(float& value, const var_info& info)
{
	std::string line;
	SerialiseText<float>(value, info, line);
	if (!line.empty())
		value = std::stof(line);
}

void Archiver::Serialise(uint16_t& value, const var_info& info)
{
	std::string line;
	SerialiseText<int16_t>(value, info, line);
	if (!line.empty())
		value = std::stoi(line);
}

void Archiver::Serialise(int& value, const var_info& info)
{
	std::string line;
	SerialiseText<int>(value, info, line);
	if (!line.empty())
		value = std::stoi(line);
}

void Archiver::Serialise(uint32_t& value, const var_info& info)
{
	std::string line;
	SerialiseText<uint32_t>(value, info, line);
	if (!line.empty())
		value = std::stoi(line);
}

void Archiver::Serialise(long& value, const var_info& info)
{
	std::string line;
	SerialiseText<long>(value, info, line);
	if (!line.empty())
		value = std::stol(line);
}

void Archiver::Serialise(bool& value, const var_info& info)
{
	std::string line;
	SerialiseText<bool>(value, info, line);
	if (!line.empty())
		value = (std::stoi(line) == 0) ? false : true;
}

void Archiver::Serialise(double& value, const var_info& info)
{
	std::string line;
	SerialiseText<double>(value, info, line);
	if (!line.empty())
		value = (std::stod(line) == 0) ? false : true;
}

// glm functions
void Archiver::Serialise(glm::vec2& vec, const var_info& info)
{
	Serialise(vec.x, var_info(info.name + ".x"));
	Serialise(vec.y, var_info(info.name + ".y"));
}

void Archiver::Serialise(glm::vec3& vec, const var_info& info)
{
	Serialise(vec.x, var_info(info.name + ".x"));
	Serialise(vec.y, var_info(info.name + ".y"));
	Serialise(vec.z, var_info(info.name + ".z"));
}

void Archiver::Serialise(glm::vec4& vec, const var_info& info)
{
	Serialise(vec.x, var_info(info.name + ".x"));
	Serialise(vec.y, var_info(info.name + ".y"));
	Serialise(vec.z, var_info(info.name + ".z"));
	Serialise(vec.w, var_info(info.name + ".w"));
}

void Archiver::Serialise(glm::mat4& vec, const var_info& info)
{
	Serialise(vec[0], var_info(info.name + "[0].mat4"));
	Serialise(vec[1], var_info(info.name + "[1].mat4"));
	Serialise(vec[2], var_info(info.name + "[2].mat4"));
	Serialise(vec[3], var_info(info.name + "[3].mat4"));
}

