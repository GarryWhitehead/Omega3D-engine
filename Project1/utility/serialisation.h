#pragma once
#include "utility/file_log.h"
#include <fstream>
#include <string>
#include <memory>
#include <unordered_map>
#include "glm.hpp"

enum class FileMode
{
	FILE_IN,
	FILE_OUT
};

enum class SaveMode
{
	SAVE_TEXT,
	SAVE_BINARY
};

class Archiver
{
public:

	struct var_info
	{
		var_info(const std::string& n) : name(n) {}

		std::string name;
	};

	Archiver();

	// regular variable types
	void Serialise(char& value, const var_info& info);
	void Serialise(float& value, const var_info& info);
	void Serialise(int& value, const var_info& info);
	void Serialise(uint16_t& value, const var_info& info);
	void Serialise(uint32_t& value, const var_info& info);
	//void Serialise(int32_t& value, const var_info& info);
	void Serialise(double& value, const var_info& info);
	void Serialise(long& value, const var_info& info);
	void Serialise(bool& value, const var_info& info);
	void Serialise(std::string& str, const var_info& info);

	// vector serialisation
	template <typename T>
	void Serialise(std::vector<T>& vec, const var_info& info);

	// glm-based serialisation
	void Serialise(glm::vec2& vec, const var_info& info);
	void Serialise(glm::vec3& vec, const var_info& info);
	void Serialise(glm::vec4& vec, const var_info& info);
	void Serialise(glm::mat4& vec, const var_info& info);

	void OpenFile(const std::string filename, SaveMode saveMode, FileMode fileMode);

private:

	// serialise data to/from text
	template <typename T>
	void SerialiseText(T value, const var_info& info, std::string& line);

	
	FileMode mode;
	std::fstream file;
};

template <typename T>
void Archiver::Serialise(std::vector<T>& vec, const var_info& info)
{
	uint32_t vecSize = static_cast<uint32_t>(vec.size());
	Serialise(vecSize, var_info(info.name + ".size"));
	vec.resize(vecSize);

	for (uint32_t c = 0; c < vecSize; ++c) {

		Serialise(vec[c], var_info(info.name + "[" + std::to_string(c) + "]"));
	}
}

template <typename T>
void Archiver::SerialiseText(T value, const var_info& info, std::string& line)
{
	// data stored in text file in the format e.g. <.x>val
	if (mode == FileMode::FILE_OUT) {

		std::string begin_str = "<" + info.name + ">";
		std::string end_str = "";

		file << begin_str << value << end_str << "\n";
	}
	else {
		
		getline(file, line);
		size_t pos = line.find_last_of('>');
		if (pos != std::string::npos) {

			line = line.substr(pos + 1, line.size());
		}
		else {

			*g_filelog << "Critical error! Unable to read data from level file. Incorrect format. Exiting......";
			exit(EXIT_FAILURE);
		}
	}
}




