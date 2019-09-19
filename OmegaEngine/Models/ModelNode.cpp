#include "ModelNode.h"

namespace OmegaEngine
{
ModelNode::ModelNode()
{
}

ModelNode::~ModelNode()
{
}

bool ModelNode::prepareExtensions(const cgltf_extras& extras, cgltf_data& data)
{
	// first check whther there are any extensions
	cgltf_size extSize;
	cgltf_copy_extras_json(&data, &extras, nullptr, &extSize);
	if (!extSize)
	{
		return false;
	}

	// alloc mem for the json extension data
	char* jsonData = new char[extSize + 1];

	cgltf_result res = cgltf_copy_extras_json(&data, &extras, jsonData, &extSize);
	if (!res)
	{
		LOGGER_ERROR("Unable to prepare extension data. Error code: %d\n", res);
		return false;
	}

	jsonData[extSize] = '\0';

	// create json file from data blob - using cgltf implementation here
	jsmn_parser jsonParser;
	jsmn_init(&jsonParser);
	const I count = jsmn_parse(&jsonParser, jsonData, extSize, nullptr, 0);
	if (count < 1)
	{
		LOGGER_ERROR("Unable to parse extensions json file.\n");
		return false;
	}

	return true;
}

}    // namespace OmegaEngine