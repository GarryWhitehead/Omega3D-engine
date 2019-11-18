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
    
	struct Dimensions
	{
		OEMaths::vec3f min;
		OEMaths::vec3f max;
		OEMaths::vec3f size;
		OEMaths::vec3f center;
		float radius;

		//void initDimensions(OEMaths::vec3f min, OEMaths::vec3f max);
	};
    
    /**
     * @brief A abstract vertex buffer. Vertices are stored as blobs of data as the shader system allows
     * numerous variants in the vertex inputs. The blob is described by the **VertexDescriptor*
     */
	struct VertexBuffer
	{
        /**
         * Specifies a variant to use when compiling the shader
         */
        enum class Variant : uint64_t
        {
            HasSkin,
            TangentInput,
            BiTangentInput,
            HasUv,
            HasNormal,
            HasWeight,
            HasJoint
        };
        
        uint8_t* data = nullptr;
        size_t size = 0;
        std::vector<Attribute> attributes;
        BitSetEnum<Variant> variants;
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
	std::vector<VertexBufferr> vertices;
    
    /// All indices associated with this particular model
	std::vector<uint32_t> indices;
};

}    // namespace OmegaEngine
