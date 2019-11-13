#pragma once

#include "OEMaths/OEMaths.h"

#include "Models/Formats/GltfModel.h"

#include "assimp/include/assimp/scene.h"

#include "utility/BitSetEnum.h"

#include "cgltf/cgltf.h"

#include <vector>


namespace OmegaEngine
{

// forward declerations
enum class StateTopology;

class ModelMesh
{
public:
    
    /**
     * Specifies a variant to use when compiling the shader
     */
    enum class Variants : uint64_t
    {
        Skinning,
        TangentInput,
        BiTangentInput,
    };
    
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
        OEMaths::vec3f tangent;
        OEMaths::vec3f bitangent;
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

	/// defines the topology to use in the program state
	StateTopology topology;
    
    /// the overall dimensions of this model. Sub-meshses contain their own dimension data
	Dimensions totalDimensions;
    
    /// this is used for shader variants
    BitSetEnum<Variants> variants;
    
    /// sub-meshes
	std::vector<Primitive> primitives;
    
    /// All vertivces associated with the particular model
	std::vector<Vertex> vertices;
    
    /// All indices associated with this particular model
	std::vector<uint32_t> indices;
};

}    // namespace OmegaEngine
