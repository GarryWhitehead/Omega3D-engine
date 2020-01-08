#pragma once

#include "utility/CString.h"

#include "OEMaths/OEMaths.h"

#include <vector>

namespace OmegaEngine
{
class SkinInstance;
class MaterialInstance;

class MeshInstance
{
public:

	MeshInstance() = default;

	AABBox& getAABBox();

	MaterialInstance* getMaterial();

};

class NodeInstance
{
public:

	NodeInstance();
	~NodeInstance();

	MeshInstance* getMesh();

	SkinInstance* getSkin();
};

class GltfModel
{

public:

	GltfModel();

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

private:

};

}    // namespace OmegaEngine