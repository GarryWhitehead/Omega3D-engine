#pragma once

#include "OEMaths/OEMaths.h"

#include "Models/Formats/GltfModel.h"

#include "assimp/include/assimp/scene.h"

#include "cgltf/cgltf.h"

#include <vector>


namespace OmegaEngine
{

// forward declerations
enum class StateTopology;

class ModelMesh
{
public:
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
		OEMaths::vec2f uv;
		OEMaths::vec3f normal;
		OEMaths::vec4f weight;
		OEMaths::vec4f joint;
	};

	struct Primitive
	{
		Primitive() = default;

		Dimensions dimensions;
		int32_t materialId = -1;  ///< set once added to the renderable manager

		// index offsets
		uint32_t indexBase = 0;
		uint32_t indexCount = 0;
	};

	ModelMesh() = default;

	bool prepare(const cgltf_mesh& mesh, GltfModel& model);

	bool prepare(aiScene* scene);

	friend class RenderableManager;

private:

	// defines the topology to use in the program state
	StateTopology topology;

	Dimensions totalDimensions;

	std::vector<Primitive> primitives;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	bool skinned = false;
};

}    // namespace OmegaEngine
