#pragma once

#include "Models/Formats/GltfExtension.h"

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

class GltfModel
{

public:
	
	/// atributes
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
    
    std::vector<NodeInstance> getNodes();
    
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

    // all the extensions available for this model
    GltfExtension extensions;
    
	// world co-ords
	OEMaths::vec3f wTrans;
	OEMaths::vec3f wScale = OEMaths::vec3f{1.0f};
	OEMaths::quatf wRotation;
};

}    // namespace OmegaEngine
