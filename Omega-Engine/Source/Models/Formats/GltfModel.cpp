#include "GltfModel.h"

#include "utility/CString.h"
#include "utility/FileUtil.h"
#include "utility/Logger.h"

#include "Core/World.h"

#include "Models/AnimInstance.h"
#include "Models/MaterialInstance.h"
#include "Models/MeshInstance.h"
#include "Models/NodeInstance.h"
#include "Models/SkinInstance.h"


#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

namespace OmegaEngine
{

OEMaths::vec3f GltfExtension::tokenToVec3(Util::String str)
{
	OEMaths::vec3f output;
	auto split = Util::String::split(str, ' ');
	assert(split.size() == 3);
	return OEMaths::vec3f(split[0].toFloat(), split[1].toFloat(), split[2].toFloat());
}

Util::String GltfExtension::find(Util::String ext)
{
	auto iter = extensions.find(ext.c_str());
	if (iter == extensions.end())
	{
		return "";
	}
	return iter->second;
}

bool GltfExtension::prepare(const cgltf_extras& extras, cgltf_data& data)
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
		Util::String startStr(Util::String::valueToString(&jsonData[token.start]));
		Util::String endStr(Util::String::valueToString(token.end - token.start));
		temp.emplace_back(startStr);
		temp.emplace_back(endStr);
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
		extensions.emplace(temp[i].c_str(), temp[i + 1]);
	}

	return true;
}

// =====================================================================================================================================================

GltfModel::GltfModel()
    : extensions(std::make_unique<GltfExtension>())
{

}

NodeInfo* GltfModel::getNode(Util::String id)
{
	NodeInfo* foundNode = nullptr;
	for (NodeInstance node : nodes)
	{
		foundNode = node.getNode(id);
		if (foundNode)
		{
			break;
		}
	}
	return foundNode;
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

void GltfModel::lineariseRecursive(cgltf_node& node, size_t index)
{
	// nodes a lot of the time don't possess a name, so we can't rely on this
	// for identifying nodes. So. we will use a stringifyed id instead
	Util::String indexStr(Util::String::valueToString(index));
	node.name = indexStr.c_str();

	linearisedNodes.emplace_back(&node);

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

	return true;
}

bool GltfModel::prepare()
{
	if (!gltfData)
	{
		LOGGER_ERROR("Looks like you need to load a gltf file before calling this function!\n");
		return false;
	}

	// joints and animation samplers point at nodes in the hierachy. To link our node hierachy
	// the model nodes have there ids. We also linearise the cgltf nodes in a map, along with an
	// id which matches the pattern of the model nodes. To find a model node from a cgltf node -
	// find the id of the cgltf in the linear map, and then search for this id in the model node
	// hierachy and return the pointer.
	lineariseNodes(gltfData);

	// for each scene, visit each node in that scene
	cgltf_scene* sceneEnd = gltfData->scenes + gltfData->scenes_count;
	for (const cgltf_scene* scene = gltfData->scenes; scene < sceneEnd; ++scene)
	{
		cgltf_node* const* nodeEnd = scene->nodes + scene->nodes_count;
		for (cgltf_node* const* node = scene->nodes; node < nodeEnd; ++node)
		{
			NodeInstance newNode;
			if (!newNode.prepare(*node, *this))
			{
				return false;
			}
		}
	}

	return true;
}

GltfExtension& GltfModel::getExtensions()
{
	return *extensions;
}

// ================ user front-end functions =========================

GltfModel& GltfModel::setWorldTrans(const OEMaths::vec3f trans)
{
	wTrans = trans;
	return *this;
}


GltfModel& GltfModel::setWorldScale(const OEMaths::vec3f scale)
{
	wScale = scale;
	return *this;
}

GltfModel& GltfModel::setWorldRotation(const OEMaths::quatf rot)
{
	wRotation = rot;
	return *this;
}

std::vector<NodeInstance> GltfModel::getNodes()
{
	return nodes;
}

}    // namespace OmegaEngine
