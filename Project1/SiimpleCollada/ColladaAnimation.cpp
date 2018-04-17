#include "ColladaAnimation.h"



ColladaAnimation::ColladaAnimation(XMLparse *parse) :
	m_xmlParse(parse)
{
}


ColladaAnimation::~ColladaAnimation()
{
}

void ColladaAnimation::ImportLibraryAnimations()
{
	XMLPbuffer buffer;

	buffer = m_xmlParse->ReadTreeIntoBuffer("library_animations");
	if (m_xmlParse->ReportErrors() == ErrorFlags::XMLP_INCORRECT_FILE_FORMAT) {

		return;
	}

	// before stating, evaluate the type of transform used - X translation, thus only one float, or full transform, then using 4x4 matrix
	// This is found in the "technique_common" node in the child node param name or by checking the stride in accessor source
	uint32_t index = 0;
	index = m_xmlParse->FindElementInBuffer("technique_common", buffer, index);
	std::string type = m_xmlParse->ReadElementDataString("type", buffer, index + 2);		// making the assumpton that all data is laid out the same
	transformType = (type == "float") ? TransformType::TRANSFORM_FLOAT : TransformType::TRANSFORM_4X4;

	index = 0;
	while (index < buffer.size()) {

		index = m_xmlParse->FindElementInBuffer("sampler id", buffer, index);
		if (index != UINT32_MAX) {

			// there are three types of input - INPUT which gives timing info; OUPUT - which gives the transform data at that particlaur time point
			// and INTERPOLATION - which gives the type of intrepolation to use between values (usually LINEAR)
			std::string  semantic;
			InputSemanticInfo semanticInfo;

			++index;
			for (int c = 0; c < 3; ++c) {

				semantic = m_xmlParse->ReadElementDataString("input semantic", buffer, index);
				if (!semantic.empty()) {

					if (semantic == "INPUT") {
						semanticInfo.input = m_xmlParse->ReadElementDataString("source", buffer, index++);
					}
					else if (semantic == "OUTPUT") {
						semanticInfo.output = m_xmlParse->ReadElementDataString("source", buffer, index++);
					}
					else if (semantic == "INTERPOLATION") {
						semanticInfo.interpolation = m_xmlParse->ReadElementDataString("source", buffer, index++);
					}
				}
			}
			m_semanticData.push_back(semanticInfo);
		}
	}

	if (transformType == TransformType::TRANSFORM_4X4) {

		GetAnimationData4X4Transform(buffer);
	}
	else {
		GetAnimationDataFloatTransform(buffer);
	}

}

void ColladaAnimation::GetAnimationDataFloatTransform(XMLPbuffer &buffer)
{
	// now we have the input semantic info - we can now read the corresponding data
	uint32_t index = 0;
	uint32_t semanticIndex = 0;

	while (index < buffer.size()) {

		LibraryAnimationFloatInfo animInfo;

		for (int c = 0; c < 3; ++c) {

			index = m_xmlParse->FindElementInBuffer("source id", buffer, index);
			if (index == UINT32_MAX) {
				return;
			}

			std::string id = m_xmlParse->ReadElementDataString("source id", buffer, index++);

			uint32_t arrayCount = 0;
			if (id == m_semanticData[semanticIndex].input) {

				arrayCount = GetFloatArrayInfo(m_semanticData[semanticIndex].input, buffer, index, semanticIndex);
				index = m_xmlParse->ReadElementArray<float>(buffer, animInfo.time, index, arrayCount);
			}
			else if (id == m_semanticData[semanticIndex].output) {

				arrayCount = GetFloatArrayInfo(m_semanticData[semanticIndex].output, buffer, index, semanticIndex);
				index = m_xmlParse->ReadElementArray<float>(buffer, animInfo.transform, index, arrayCount);
			}
		}

		libraryAnimationFloatData.push_back(animInfo);
		++semanticIndex;
	}
}

void ColladaAnimation::GetAnimationData4X4Transform(XMLPbuffer &buffer)
{
	// now we have the input semantic info - we can now read the corresponding data
	uint32_t index = 0;
	uint32_t semanticIndex = 0;

	LibraryAnimation4X4Info animInfo;

	while (index < buffer.size()) {

		for (int c = 0; c < 3; ++c) {

			index = m_xmlParse->FindElementInBuffer("source id", buffer, index);
			if (index == UINT32_MAX) {
				return;
			}

			std::string id = m_xmlParse->ReadElementDataString("source id", buffer, index++);

			uint32_t arrayCount = 0;
			if (id == m_semanticData[semanticIndex].input) {

				arrayCount = GetFloatArrayInfo(m_semanticData[semanticIndex].input, buffer, index, semanticIndex);
				index = m_xmlParse->ReadElementArray<float>(buffer, animInfo.time, index, arrayCount);
			}
			else if (id == m_semanticData[semanticIndex].output) {

				arrayCount = GetFloatArrayInfo(m_semanticData[semanticIndex].output, buffer, index, semanticIndex);
				index = m_xmlParse->ReadElementArrayMatrix(buffer, animInfo.transform, index, arrayCount);
			}
		}

		libraryAnimation4x4Data.push_back(animInfo);
		++semanticIndex;
	}
}

uint32_t ColladaAnimation::GetFloatArrayInfo(std::string id, XMLPbuffer& buffer, uint32_t index, uint32_t semanticIndex)
{
	// start by ensuring that this is a float array
	std::string arrayId = m_xmlParse->ReadElementDataString("float_array id", buffer, index);
	if (arrayId != id + "-array") {
		return 0;
	}

	uint32_t arrayCount = m_xmlParse->ReadElementDataInt("count", buffer, index);
	return arrayCount;
}


