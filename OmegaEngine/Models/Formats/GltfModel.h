#pragma once
#include "Models/ModelAnimation.h"
#include "Models/ModelImage.h"
#include "Models/ModelMaterial.h"
#include "Models/ModelMesh.h"
#include "Models/ModelNode.h"
#include "Models/ModelSkin.h"

#include "utility/String.h"

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
	* @ loads a specified gltf file and parses the data
	* @ param absolute path to the gltf model file. Binary data must also be present in the directory
	* @ return Whether the file was succesfully loaded and parsed
	*/
	bool load(Util::String filename);

	// helper functions
	/**
	* @ brief Adds a matrial to the container after checking for duplicate.
	* If a duplicate, returns the index were the duplicate is located
	* Otherwise adds the material and returns a index to this material
	* @param The prepared material to add
	*/
	size_t addMaterial(ModelMaterial& mat);

	/**
	* @brief Adds a new skin to the container
	* @param skin New skin to add
	* @return index where the new skin has been added
	*/
	size_t addSkin(ModelSkin& skin);
    
    /**
     * @brief Trys to find a node in a heirachy.
     * @param id The id of the node to find.
     * @return If successful, returns a pointer to the node. Otherwise returns nullptr.
     */
    ModelNode* getNode(size_t id);
    
	friend class Scene;

private:

	void lineariseRecursive(cgltf_node& node, size_t index);
	void lineariseNodes(cgltf_data* data);

private:
	std::vector<ModelNode> nodes;

	// materials and image paths pulled out of the nodes.
	std::vector<ModelMaterial> materials;

	// skeleton data also removed from the nodes
	std::vector<ModelSkin> skins;

	std::vector<ModelAnimation> animations;

	// linearised nodes - with the name updated to store an id
    // for linking to our own node hierachy
	std::vector<cgltf_node*> linearisedNodes;

	
};

}    // namespace OmegaEngine
