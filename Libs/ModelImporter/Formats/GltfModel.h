#pragma once

#include "ModelImporter/AnimInstance.h"
#include "ModelImporter/MaterialInstance.h"
#include "ModelImporter/MeshInstance.h"
#include "ModelImporter/NodeInstance.h"
#include "ModelImporter/SkinInstance.h"

#include "utility/CString.h"

#include "OEMaths/OEMaths.h"

#include <memory>
#include <unordered_map>
#include <vector>

#include "cgltf/cgltf.h"

namespace OmegaEngine
{
// forward declerations
class NodeInstance;
class MeshInstance;
class AnimInstance;
class SkinInstance;
class MaterialInstance;
struct NodeInfo;

// <extension name, value (as string)>
using ExtensionData = std::unordered_map<const char*, Util::String>;

class GltfExtension
{
public:
	// utility functions for dealing with gltf json data
	bool prepare(const cgltf_extras& extras, cgltf_data& data);

	/// token to string converion functions
	static OEMaths::vec3f tokenToVec3(Util::String str);

	Util::String find(Util::String ext);

private:
	std::unordered_map<const char*, Util::String> extensions;
};

class GltfModel
{

public:
	
	GltfModel();

	/// atributes
	static uint8_t* getAttributeData(const cgltf_attribute* attrib, size_t& stride);

	/**
	* @ loads a specified gltf file
	* @ param absolute path to the gltf model file. Binary data must also be present in the directory
	* @ return Whether the file was succesfully loaded
	*/
	bool load(Util::String filename);

	/**
	* @brief Parses the file that was loaded in via **load**
	* Note: You must call **load** before this function otherwise you will get an error.
	*/
	bool prepare();

	// =========== helper functions ================
    /**
     * @brief Trys to find all node heirachies.
     * @param id The id of the node to find.
     * @return If successful, returns a pointer to the node. Otherwise returns nullptr.
     */
	NodeInfo* getNode(Util::String id);
    
    GltfExtension& getExtensions();
    
    // ========= user front-end functions ==============
    
	/**
	* @brief Sets the world translation for this model
	*/
	GltfModel& setWorldTrans(const OEMaths::vec3f trans);

	/**
	* @brief Sets the world scale for this model
	*/
	GltfModel& setWorldScale(const OEMaths::vec3f scale);

	/**
	* @brief Sets the world rotation for this model
	*/
	GltfModel& setWorldRotation(const OEMaths::quatf rot);
    
	void setDirectory(Util::String dir);

private:

	void lineariseRecursive(cgltf_node& node, size_t index);
	void lineariseNodes(cgltf_data* data);


public:

	// ======================= model data (public) =======================================

	std::vector<std::unique_ptr<NodeInstance>> nodes;

	// materials and image paths pulled out of the nodes.
	std::vector<MaterialInstance> materials;

	// skeleton data also removed from the nodes
	std::vector<SkinInstance> skins;

	std::vector<AnimInstance> animations;

private:

	// ======================= model data (private) =======================================

	cgltf_data* gltfData = nullptr;

	// linearised nodes - with the name updated to store an id
    // for linking to our own node hierachy
	std::vector<cgltf_node*> linearisedNodes;

    // all the extensions available for this model
    std::unique_ptr<GltfExtension> extensions;
    
	// world co-ords
	OEMaths::vec3f wTrans;
	OEMaths::vec3f wScale = OEMaths::vec3f{1.0f};
	OEMaths::quatf wRotation;

	// user defined path to the model directory
	Util::String modelDir;
};

}    // namespace OmegaEngine
