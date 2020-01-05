#include "GltfModel.h"

#include "utility/FileUtil.h"
#include "utility/Logger.h"
#include "utility/CString.h"

#include "Core/World.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"

namespace OmegaEngine
{

NodeInstance::NodeInfo* GltfModel::getNode(Util::String id)
{
    NodeInstance::NodeInfo* foundNode = nullptr;
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
    
size_t GltfModel::addMaterial(MaterialInstance& mat)
{
	// check for duplicate materials first. This is possible if there are lots of primitives
	Util::String MatName = mat.getName();

	size_t index = 0;
	for (MaterialInstance& modelMat : materials)
	{
		if (MatName.compare(modelMat.getName()))
		{
			return index;
		}
		++index;
	}

	materials.emplace_back(mat);
	return index + 1;
}

size_t GltfModel::addSkin(SkinInstance& skin)
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
            
            // now prepare the skins. This requires finding the nodes which are joints, and
            // adding the index values to the nodes. We expect one skin per mesh (of which there
            // can only be one per node!)
            for (auto& skin : newNode.skins)
            {
                SkinInstance newSkin;
                newSkin.prepare(*skin, newNode);
                skins.emplace_back(newSkin);
            }
		}
	}
    
	return true;
}

GltfExtension& GltfModel::getExtensions()
{
    return extensions;
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

MeshInstance* GltfModel::getMesh()
{
    
}

SkinInstance* GltfModel::getSkin()
{
    
}

NodeInstance* GltfModel::getNode()
{
    
}
   
MaterialInstance* GltfModel::getMaterial()
{
    
}



}    // namespace OmegaEngine
