#pragma once

#include "OEMaths/OEMaths.h"

#include "Types/AABox.h"

#include "VulkanAPI/Common.h"

#include "ModelImporter/Formats/GltfModel.h"
#include "Core/Omega_Common.h"

#include "utility/BitsetEnum.h"

#include "assimp/scene.h"

#include <vector>

namespace OmegaEngine
{

class MeshInstance
{
public:
    
    /**
     * @brief Specifies a variant to use when compiling the shader
     */
    enum class Variant : uint64_t
    {
        HasSkin,
        TangentInput,
        BiTangentInput,
        HasUv,
        HasNormal,
        HasWeight,
        HasJoint,
        __SENTINEL__
    };
    
	
	/**
     * @brief A abstract vertex buffer. Vertices are stored as blobs of data as the shader system allows
     * numerous variants in the vertex inputs. The blob is described by the **VertexDescriptor*
     */
	struct VertexBuffer
	{
		enum Attribute
        {
            attr_float,
            attr_int,
            attr_vec2,
            attr_vec3,
            attr_vec4,
            atrr_mat3,
            attr_mat4
        };
        
        struct Descriptor
        {
            uint8_t stride;
            uint8_t width;
        };

        uint8_t* data = nullptr;
		size_t size = 0;
		std::vector<Attribute> attributes;
	};

	struct Primitive
	{
		Primitive() = default;

        AABBox dimensions;

		// index offsets
		size_t indexBase = 0;
		size_t indexCount = 0;

		// ============ vulakn backend ==========================
		// set by calling **update**
		size_t indexPrimitiveOffset;    // this equates to buffer_offset + sub-offset
		size_t indexPrimitiveCount;
	};

	MeshInstance() = default;

	bool prepare(const cgltf_mesh& mesh, GltfModel& model);

	bool prepare(aiScene* scene);

    AABBox& getAABBox()
    {
        return dimensions;
    }
    
    MaterialInstance* getMaterial();
    
	friend class RenderableManager;
	friend class GBufferFillPass;
    friend class OEScene;

private:
    
    /// the material associated with this mesh (if it has one)
    MaterialInstance* material = nullptr;
    
	/// defines the topology to use in the program state
	vk::PrimitiveTopology topology;

	/// the overall dimensions of this model. Sub-meshses contain their own dimensions
    AABBox dimensions;

	/// sub-meshes
	std::vector<Primitive> primitives;

	/// All vertivces associated with the particular model
	VertexBuffer vertices;

	/// All indices associated with this particular model
	std::vector<uint32_t> indices;
    
    /// variation of the mesh shader
    Util::BitSetEnum<Variant> variantBits;
    
    // for debugging purposes - do not use
    cgltf_material* cached_mat_ptr = nullptr;
};

}    // namespace OmegaEngine
