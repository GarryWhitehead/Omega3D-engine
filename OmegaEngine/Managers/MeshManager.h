#pragma once
#include "Types/Object.h"

#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"

#include "Utility/logger.h"

#include <memory>
#include <tuple>
#include <unordered_map>

#define MESH_INIT_CONTAINER_SIZE 50

namespace OmegaEngine
{
// forard decleartions
class ObjectManager;
class Object;
struct MeshComponent;
enum class StateTopolgy;
enum class StateMesh;

struct PrimitiveMesh
{
	StateMesh type;

	// index offsets
	uint32_t indexBase = 0;
	uint32_t indexCount = 0;

	// material id
	uint32_t materialId;
};

struct StaticMesh
{
	StateMesh type;

	// primitives assoicated with this mesh
	std::vector<PrimitiveMesh> primitives;

	// states
	StateTopology topology;

	// offset into mega buffer
	uint32_t vertexBufferOffset;
	uint32_t indexBufferOffset;
};

class MeshManager
{

public:
	// a user-defined size for the vertex and index gpu mem blocks - this should maybe made more dynamic? Also needs checking for overspill....
	static constexpr float VertexBlockSize = 1e+5;
	static constexpr float IndexBlockSize = 1e+5;

	struct Dimensions
	{
		OEMaths::vec3f min;
		OEMaths::vec3f max;
		OEMaths::vec3f size;
		OEMaths::vec3f center;
		float radius;

		//void initDimensions(OEMaths::vec3f min, OEMaths::vec3f max);
	};

	struct Vertex
	{
		OEMaths::vec4f position;
		OEMaths::vec2f uv0;
		OEMaths::vec2f uv1;
		OEMaths::vec3f normal;
	};

	struct SkinnedVertex
	{
		OEMaths::vec4f position;
		OEMaths::vec2f uv0;
		OEMaths::vec2f uv1;
		OEMaths::vec3f normal;
		OEMaths::vec4f weight;
		OEMaths::vec4f joint;
	};

	MeshManager();
	~MeshManager();

	/// called on a per-frame basis 
	void updateFrame();

	void addMesh(std::unique_ptr<ModelMesh>& mesh, Object& obj);

	void linkMaterialWithMesh(MeshComponent* meshComponent, MaterialComponent* materialComponent);

	StaticMesh& getMesh(MeshComponent& comp)
	{
		assert(comp.index < meshBuffer.size());
		return meshBuffer[comp.index];
	}

private:
	// the buffers containing all the model data
	std::vector<StaticMesh> meshBuffer;

	// all vertices and indices held in one large buffer
	std::vector<Vertex> staticVertices;
	std::vector<SkinnedVertex> skinnedVertices;
	std::vector<uint32_t> indices;

	bool isDirty = true;
};

}    // namespace OmegaEngine
