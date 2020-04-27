/* Copyright (c) 2018-2020 Garry Whitehead
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "OEMaths/OEMaths.h"
#include "assimp/scene.h"
#include "cgltf/cgltf.h"
#include "utility/BitsetEnum.h"

#include <limits>
#include <vector>

namespace OmegaEngine
{
class GltfModel;
class MaterialInstance;

enum class Topology
{
    PointList,
    LineList,
    LineStrip,
    TrinagleList,
    TriangleStrip,
    TriangleFan,
    PatchList,
    Undefined
};

class MeshInstance
{
public:
    struct Dimensions
    {
        Dimensions()
            : min(OEMaths::vec3f {std::numeric_limits<float>::max()})
            , max(OEMaths::vec3f {std::numeric_limits<float>::min()})
        {
        }
        OEMaths::vec3f min;
        OEMaths::vec3f max;
    };

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
     * @brief A abstract vertex buffer. Vertices are stored as blobs of data as the shader system
     * allows numerous variants in the vertex inputs. The blob is described by the
     * **VertexDescriptor*
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
        uint64_t size = 0;
        uint32_t strideSize = 0;
        uint32_t vertCount = 0;
        std::vector<Attribute> attributes;
    };

    struct Primitive
    {
        Primitive() = default;

        // sub-mesh dimensions.
        Dimensions dimensions;

        // index offsets
        size_t indexBase = 0;
        size_t indexCount = 0;

        // ============ vulakn backend ==========================
        // set by calling **update**
        size_t indexPrimitiveOffset = 0; // this equates to buffer_offset + sub-offset
    };

    MeshInstance() = default;
    ~MeshInstance();

    bool prepare(const cgltf_mesh& mesh, GltfModel& model);

    bool prepare(aiScene* scene);


    // ======================= mesh data (public) =======================================

    /// the material associated with this mesh (if it has one)
    MaterialInstance* material = nullptr;

    /// defines the topology to use in the program state
    Topology topology = Topology::TrinagleList;

    /// the overall dimensions of this model. Sub-meshses contain their own dimensions
    Dimensions dimensions;

    /// sub-meshes
    std::vector<Primitive> primitives;

    /// All vertivces associated with the particular model
    VertexBuffer vertices;

    /// All indices associated with this particular model
    std::vector<uint32_t> indices;

    /// variation of the mesh shader
    Util::BitSetEnum<Variant> variantBits;

private:
    // ===================== mesh data (private) =======================================

    // for debugging purposes - do not use
    cgltf_material* cached_mat_ptr = nullptr;
};

} // namespace OmegaEngine
