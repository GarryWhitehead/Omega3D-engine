#include "GltfModel.h"

#include "Utility/FileUtil.h"
#include "Utility/logger.h"
#include "utility/String.h"

#include "Core/World.h"

namespace OmegaEngine
{

size_t GltfModel::addMaterial(ModelMaterial& mat)
{
	// check for duplicate materials first. This is possible if there are lots of primitives
	Util::String matName = mat.getName();

	size_t index = 0;
	for (ModelMaterial& modelMat : materials)
	{
		if (matName.compare(modelMat.getName()))
		{
			return index;
		}
		++index;
	}

	materials.emplace_back(mat);
	return index + 1;
}

size_t GltfModel::addSkin(ModelSkin& skin)
{
	skins.emplace_back(skin);
	return skins.size() - 1;
}

void GltfModel::getAttributeData(const cgltf_attribute* attrib, uint8_t* base, size_t& stride)
{
	const cgltf_accessor* accessor = attrib->data;
	base = static_cast<uint8_t*>(accessor->buffer_view->buffer->data);    // total data blob
	stride = accessor->buffer_view->stride;                               // the size of each sub blob
	if (!stride)
	{
		stride = accessor->stride;
	}
	assert(stride);
}

OEMaths::vec3f GltfModel::tokenToVec3(Util::String str)
{
	OEMaths::vec3f output;
	auto split = str.split(' ');
	assert(split.size() == 3);
	return OEMaths::vec3f(split[0].toFloat(), split[1].toFloat(), split[2].toFloat());
}

bool GltfModel::prepareExtensions(const cgltf_extras& extras, cgltf_data& data, ExtensionData& extensions)
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
	const int tokenCount = jsmn_parse(&jsonParser, jsonData, extSize, nullptr, 0);
	if (tokenCount < 1)
	{
		LOGGER_ERROR("Unable to parse extensions json file.\n");
		return false;
	}

	// we know the token size, so allocate memory for this and get the tokens
	std::vector<jsmntok_t> tokenData;
	jsmn_parse(&jsonParser, jsonData, extSize, tokenData.data(), tokenCount);

	// now convert everything to a string for easier storage
	std::vector<Util::String> temp;
	for (const jsmntok_t& token : tokenData)
	{
		Util::String str(&jsonData[token.start], token.end - token.start);
		temp.emplace_back(str);
	}

	// should be divisible by 2 otherwise we will overflow when creating the output
	if ((temp.size() % 2) != 0)
	{
		LOGGER_ERROR("Error while parsing tokens. Invalid size.\n");
		return false;
	}

	// create the output - <extension name, value>
	for (size_t i = 0; i < temp.size(); i += 2)
	{
		extensions.emplace(temp[i], temp[i + 1]);
	}

	return true;
}

void GltfModel::lineariseRecursive(cgltf_node& node, size_t index)
{
	linearisedNodes.emplace(&node, index++);

	cgltf_node** childEnd = node.children + node.children_count;
	for (cgltf_node* const* child = node.children; child < childEnd; ++child)
	{
		lineariseRecursive(**child, index);
	}
}

void GltfModel::lineariseNodes(cgltf_data* data)
{
	size_t index = 0;

	cgltf_scene* sceneEnd = data->scenes + data->scenes_count;
	for (const cgltf_scene* scene = data->scenes; scene < sceneEnd; ++scene)
	{
		cgltf_node* const* nodeEnd = scene->nodes + scene->nodes_count;
		for (cgltf_node* const* node = scene->nodes; node < nodeEnd; ++node)
		{
			lineariseRecursive(**node, index);
		}
	}
}

bool GltfModel::load(Util::String filename)
{
	// no additional options required
	cgltf_options options = {};
	cgltf_data* gltfData;

	cgltf_result res = cgltf_parse_file(&options, filename.c_str(), &gltfData);
	if (res != cgltf_result_success)
	{
		LOGGER_ERROR("Unable to open gltf file %s. Error code: %d\n", filename.c_str(), res);
		return false;
	}

	// the buffers need parsing separately
	res = cgltf_load_buffers(&options, gltfData, filename.c_str());
	if (res != cgltf_result_success)
	{
		LOGGER_ERROR("Unable to open gltf file data for %s. Error code: %d\n", filename.c_str(), res);
		return false;
	}

	// start by linearising the nodes. This is ued for matching nodes with an index value
	// in skeleton and animation
	lineariseNodes(gltfData);

	// for each scene, visit each node in that scene
	cgltf_scene* sceneEnd = gltfData->scenes + gltfData->scenes_count;
	for (const cgltf_scene* scene = gltfData->scenes; scene < sceneEnd; ++scene)
	{
		cgltf_node* const* nodeEnd = scene->nodes + scene->nodes_count;
		for (cgltf_node* const* node = scene->nodes; node < nodeEnd; ++node)
		{
			ModelNode newNode;
			if (!newNode.prepare(**node, OEMaths::mat4f{}, *this))
			{
				return false;
			}
		}
	}
}


}    // namespace OmegaEngine
