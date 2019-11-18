#pragma once

#include "Types/Object.h"

#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"

#include "Utility/logger.h"

#include "Models/ModelMaterial.h"
#include "Models/ModelMesh.h"

#include <memory>
#include <unordered_map>

#define MESH_INIT_CONTAINER_SIZE 50

namespace OmegaEngine
{
// forard decleartions
class GpuTextureInfo;
class ModelMesh;
class World;
class Object;
class Engine;


/**
	* @brief A convienent way to group textures together
	* supports PBR materials - if null, field not used
	*/
struct TextureGroup
{
	TextureGroup() = default;

	~TextureGroup()
	{
		for (size_t i = 0; i < TextureType::Count; ++i)
		{
			if (textures[i])
			{
				delete textures[i];
				textures[i] = nullptr;
			}
		}
	}

	// not copyable
	TextureGroup(const TextureGroup&) = delete;
	TextureGroup& operator=(const TextureGroup&) = delete;

	GpuTextureInfo* textures[TextureType::Count];
};

class RenderableManager : public ComponentManager
{

public:
	// a user-defined size for the vertex and index gpu mem blocks - this should maybe made more dynamic? Also needs checking for overspill....
	static constexpr float VertexBlockSize = 1e+5;
	static constexpr float IndexBlockSize = 1e+5;
    
	/**
	* @brief A basic struct pointing to the vertex and indices data within#
	* the larger buffer. Meshes are made up of sub-meshes or primitives
	* which reference the mesh data. They also contain an index to the material data
	* which is set when a renderable is added
	*/
	struct RenderableInstance
	{
		StateMesh type;

		// primitives assoicated with this mesh
		std::vector<ModelMesh::Primitive> primitives;

		// states
		StateTopology topology;

		// offset into mega buffer
		std::vector<ModelMesh::Vertex> vertices;
        std::vector<uint32_t> indices;
	};

	RenderableManager(Engine& engine);
	~RenderableManager();

	/// called on a per-frame basis . Updates all textures and ubos on the vullkan backend.
    /// This isn't so expensive as these are persistent resources
	void update();

	/**
	* @brief The main call here - adds a renderable consisting of mesh, and not
	* always, material and texture data. This function adds a number of materials if required
	*/
	void addRenderable(ModelMesh& mesh, ModelMaterial* mat, const size_t matCount, Object& obj);

	// === mesh related functions ===
	/**
	* @brief Adds a mesh and primitives to the manager. 
	* @param mesh The prepared mesh to add
	* @param idx The index derived from calling **getObjIndex**
	* @param offset The material buffer offset which will be added to the 
	* primitive ids. This value is obtained from **addMaterial**
	*/
	void addMesh(ModelMesh& mesh, const size_t idx, const size_t offset);

	/**
	* @breif Adds a mesh - use this overload when you want to add multiple meshes
	* linked with the same material group. 
	*/
	void addMesh(ModelMesh& mesh, Object& obj, const size_t offset);
    
    /**
     * @brief Returns a instance of a mesh based on the specified object
     */
	RenderableInstance& getMesh(Object& obj);

	// === material related functions ===
	/**
	* @brief Adds a specified number of materials to the manager
	* @param mat A pointer to a group of materials. This must not be a nullptr
	* @param count The number of materials to add
	* @return The starting index in which this group of materials is found at 
	* within the managers larger container
	*/
	size_t addMaterial(ModelMaterial* mat, const size_t count);
	
private:

	bool prepareTexture(Util::String path, GpuTextureInfo* tex);

private:
	
    Engine& engine;
    
	// the buffers containing all the model data
	std::vector<RenderableIstance> renderables;

	// all the materials
	std::vector<ModelMaterial> materials;

	// and all the textures
	std::vector<TextureGroup> textures;

    // Dirty flags - state whether anything has been added or altered and needs updating on the backend
	bool meshDirty = true;
    bool materialDirty = true;
};

}    // namespace OmegaEngie
