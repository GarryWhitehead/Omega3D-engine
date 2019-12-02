#pragma once

#include "OEMaths/OEMaths.h"

#include "Models/Formats/GltfModel.h"

#include "assimp/include/assimp/scene.h"

#include "cgltf/cgltf.h"

#include <vector>

namespace OmegaEngine
{


class MeshInstance
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
		uint8_t* data = nullptr;
		size_t size = 0;
		std::vector<VulkanAPI::Attribute> attributes;
	};

	struct Primitive
	{
		Primitive() = default;

		Dimensions dimensions;

		// index offsets
		size_t indexBase = 0;
		size_t indexCount = 0;

		// Note: this is only used for error checking.
		uint32_t materialIdx = UINT32_MAX;

		// ============ vulakn backend ==========================
		// set by calling **update**
		size_t indexPrimitiveOffset;    // this equates to buffer_offset + sub-offset
		size_t indexPrimitiveCount;
	};

	MeshInstance() = default;

	bool prepare(const cgltf_mesh& mesh, GltfModel& model);

	bool prepare(aiScene* scene);

	friend class RenderableManager;
	friend class GBufferFillPass;

private:
	/// defines the topology to use in the program state
	StateTopology topology;

	/// the overall dimensions of this model. Sub-meshses contain their own dimension data
	Dimensions totalDimensions;

	/// sub-meshes
	std::vector<Primitive> primitives;

	/// All vertivces associated with the particular model
	VertexBuffer vertices;

	/// All indices associated with this particular model
	std::vector<uint32_t> indices;
};

}    // namespace OmegaEngine
