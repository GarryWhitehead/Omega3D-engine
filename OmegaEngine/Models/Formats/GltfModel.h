#pragma once

#include "Models/AnimInstance.h"

#include "Models/Formats/GltfModel.h"
#include "Models/MaterialInstance.h"
#include "Models/MeshInstance.h"
#include "Models/NodeInstance.h"
#include "Models/SkinInstance.h"

#include "utility/CString.h"

#include "OEMaths/OEMaths.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace OmegaEngine
{

// <extension name, value (as string)>
using ExtensionData = std::unordered_map<Util::String, Util::String>;

class GltfModel
{

public:
	// utility functions for dealing with gltf json data
	static bool prepareExtensions(const cgltf_extras& extras, cgltf_data& data, ExtensionData& extensions);
	static OEMaths::vec3f tokenToVec3(Util::String str);

	// atributes
	static void getAttributeData(const cgltf_attribute* attrib, uint8_t* base, size_t& stride);

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

	// helper functions
	/**
	* @ brief Adds a matrial to the container after checking for duplicate.
	* If a duplicate, returns the index were the duplicate is located
	* Otherwise adds the material and returns a index to this material
	* @param The prepared material to add
	*/
	size_t addMaterial(MaterialInstance& mat);

	/**
	* @brief Adds a new skin to the container
	* @param skin New skin to add
	* @return index where the new skin has been added
	*/
	size_t addSkin(SkinInstance& skin);
    
    /**
     * @brief Trys to find all node heirachies.
     * @param id The id of the node to find.
     * @return If successful, returns a pointer to the node. Otherwise returns nullptr.
     */
	NodeInstance::NodeInfo* getNode(Util::String id);
    
	/**
	* @brief Sets the world translation for this model
	*/
	void setWorldTrans(OEMaths::vec3f& trans);

	/**
	* @brief Sets the world scale for this model
	*/
	void setWorldScale(OEMaths::vec3f& scale);

	/**
	* @brief Sets the world rotation for this model
	*/
	void setWorldRotation(OEMaths::quatf& rot);

	friend class Scene;

private:

	void lineariseRecursive(cgltf_node& node, size_t index);
	void lineariseNodes(cgltf_data* data);

private:

	cgltf_data* gltfData = nullptr;

	std::vector<NodeInstance> nodes;

	// materials and image paths pulled out of the nodes.
	std::vector<MaterialInstance> materials;

	// skeleton data also removed from the nodes
	std::vector<SkinInstance> skins;

	std::vector<AnimInstance> animations;

	// linearised nodes - with the name updated to store an id
    // for linking to our own node hierachy
	std::vector<cgltf_node*> linearisedNodes;

	// world co-ords
	OEMaths::vec3f wTrans;
	OEMaths::vec3f wScale = OEMaths::vec3f{1.0f};
	OEMaths::quatf wRotation;
};

}    // namespace OmegaEngine
