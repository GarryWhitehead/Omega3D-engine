#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <type_traits>
#include "glm.hpp"

enum class ErrorFlags
{
	XMLP_ERROR_UNABLE_TO_OPEN_FILE,
	XMLP_ERROR_UNABLE_TO_FIND_NODE,
	XMLP_INCORRECT_FILE_FORMAT,
	XMLP_NO_DATA_IN_SPECIFIED_NODE,
	XMLP_INCORRECT_NODE_FORMAT,
	XMLP_NO_ERROR
};

enum class VectorType
{
	VEC2_TYPE = 2,
	VEC3_TYPE = 3,
	VEC4_TYPE = 4
};

using XMLPbuffer = std::vector<std::string>;

class XMLparse
{
public:

	static const uint32_t WHOLE_ARRAY = UINT32_MAX;

	XMLparse();
	~XMLparse();

	void LoadXMLDocument(std::string filename);
	void FindElement(std::string node);
	XMLPbuffer ReadTreeIntoBuffer(std::string node);
	uint32_t FindElementInBuffer(std::string node, XMLPbuffer& buffer, uint32_t index);
	uint32_t FindElementInBufferIterateBack(std::string node, XMLPbuffer& buffer, uint32_t index);
	uint32_t FindChildElementInBuffer(std::string parent, std::string child, XMLPbuffer& buffer, uint32_t index);
	bool CheckElement(std::string id, XMLPbuffer& buffer, uint32_t index);
	bool CheckChildElement(std::string id, XMLPbuffer& buffer, uint32_t index);

	std::string ReadElementDataString(std::string nodeName, XMLPbuffer& buffer, uint32_t index);
	int ReadElementDataInt(std::string nodeName, XMLPbuffer& buffer, uint32_t index);
	float ReadElementDataFloat(std::string nodeName, XMLPbuffer& buffer, uint32_t index);
	glm::mat4 ReadElementDataMatrix(std::string nodeName, XMLPbuffer& buffer, uint32_t index);

	template <typename T>
	T ReadElementDataVec(std::string nodeName, XMLPbuffer& buffer, uint32_t index);

	template <typename T>
	uint32_t ReadElementArrayVec(XMLPbuffer& buffer, std::vector<T>& dstBuffer, uint32_t index, uint32_t arrayCount, VectorType type);
	uint32_t ReadElementArrayMatrix(XMLPbuffer& buffer, std::vector<glm::mat4>& dstBuffer, uint32_t index, uint32_t arrayCount);
	
	template <typename T>
	uint32_t ReadElementArray(XMLPbuffer& buffer, std::vector<T>& dstBuffer, uint32_t index, uint32_t arrayCount);
	

	// helper functions
	ErrorFlags ReportErrors() { return m_errorFlags; }

private:

	std::fstream m_file;

	// error flags that can be accessed by the user
	ErrorFlags m_errorFlags;
};

template <typename T>
T XMLparse::ReadElementDataVec(std::string nodeName, XMLPbuffer& buffer, uint32_t index)
{
	std::string startNode = '<' + nodeName;

	std::string str = buffer[index];

	if (str.empty()) {
		m_errorFlags = ErrorFlags::XMLP_NO_DATA_IN_SPECIFIED_NODE;
		return T(0.0);
	}

	size_t pos = str.find(startNode);
	if (pos == std::string::npos) {
		m_errorFlags = ErrorFlags::XMLP_INCORRECT_NODE_FORMAT;
		return T(0.0f);
	}

	pos = str.find_first_of('>');
	str = str.substr(pos + 1, str.size());

	std::stringstream ss(str);
	T vec;
	uint32_t vecIndex = 0;

	while (ss >> vec[vecIndex]) {

		++vecIndex;

		char next = ss.peek();
		if (next == '<') {
			break;
		}
	}

	return vec;
}

template <typename T>
uint32_t XMLparse::ReadElementArray(XMLPbuffer& buffer, std::vector<T>& dstBuffer, uint32_t index, uint32_t arrayCount)
{
	uint32_t arrayIndex = index;

	// start by removing array data from first line
	std::string str = buffer[arrayIndex];

	size_t pos = str.find_first_of(">");
	if (pos == std::string::npos) {
		m_errorFlags = ErrorFlags::XMLP_INCORRECT_NODE_FORMAT;
		return index;
	}
	std::string floatArray = str.substr(pos + 1, str.size());

	uint32_t count = 0;
	T value;

	std::stringstream ss(floatArray);
	while (ss >> value) {
			
		if (ss.fail()) {
			return arrayIndex + 1;
		}

		dstBuffer.push_back(value);
		++count;
		if (count == arrayCount) {

			return arrayIndex;
		}
	}

	// if we arrived here, the there's a problem with the file formatting
	m_errorFlags = ErrorFlags::XMLP_INCORRECT_FILE_FORMAT;
	return arrayIndex;
}

template <typename T>
uint32_t XMLparse::ReadElementArrayVec(XMLPbuffer& buffer, std::vector<T>& dstBuffer, uint32_t index, uint32_t arrayCount, VectorType type)
{
	uint32_t arrayIndex = index;
	int floatCount = (int)type;

	// start by removing array data from first line
	std::string str = buffer[arrayIndex];

	size_t pos = str.find_first_of(">");
	if (pos == std::string::npos) {
		m_errorFlags = ErrorFlags::XMLP_INCORRECT_NODE_FORMAT;
		return index;
	}
	str = str.substr(pos + 1, str.size());
	std::stringstream ss(str);
	uint32_t count = 0;

	while (arrayIndex < buffer.size()) {

		glm::vec3 vec;

		for (int x = 0; x < floatCount; ++x) {

			ss >> vec[x];
			if (ss.eof()) {

				++arrayCount;
				str = buffer[arrayCount];
				assert(arrayCount < buffer.size());
			}
		}
		dstBuffer.push_back(vec);
		++count;
		if (count == arrayCount / (int)type) {				// stride depend on type of vector
			return arrayIndex + 1;					// point to the next line in the file after the array
		}
	}

	// if we arrived here, the there's a problem with the file formatting
	m_errorFlags = ErrorFlags::XMLP_INCORRECT_FILE_FORMAT;
	return arrayIndex;
}

