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
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "cgltf/cgltf.h"
#include "utility/CString.h"

#include <cassert>
#include <cstdint>

namespace OmegaEngine
{
// forward declerations
class MeshInstance;
class GltfExtension;

enum class MaterialPipeline
{
    MetallicRoughness,
    SpecularGlosiness,
    None
};

class MaterialInstance
{
public:
    // should reflect the layout in the mrt shader
    enum TextureType : uint32_t
    {
        BaseColour,
        Normal,
        MetallicRoughness,
        Emissive,
        Occlusion,
        Count
    };


    MaterialInstance();
    ~MaterialInstance();

    Util::String getTextureUri(cgltf_texture_view& view);

    bool prepare(cgltf_material& mat, GltfExtension& extensions);

    bool prepare(aiMaterial* mat);

    static float convertToAlpha(const cgltf_alpha_mode mode);

    // helper functions
    Util::String getName()
    {
        assert(!name.empty()); // sanity check!
        return name;
    }

    /**
     * @brief The main attributes for this material.
     */
    struct MaterialBlock
    {
        OEMaths::vec4f baseColour = OEMaths::vec4f {1.0f};
        OEMaths::vec4f emissive = OEMaths::vec4f {1.0f};
        OEMaths::vec4f diffuse = OEMaths::vec4f {1.0f};
        OEMaths::vec4f specular = OEMaths::vec4f {0.0f};
        float metallic = 1.0f;
        float roughness = 1.0f;
        float alphaMask = 0.0f;
        float alphaMaskCutOff = 1.0f;
    };

    /**
     @brief the sampler details for each texture
     */
    struct Sampler
    {
        enum Filter
        {
            Nearest,
            Linear,
            Cubic
        };

        enum AddressMode
        {
            Repeat,
            MirroredRepeat,
            ClampToEdge,
            ClampToBorder,
            MirrorClampToEdge
        };

        Filter magFilter = Filter::Linear;
        Filter minFilter = Filter::Linear;
        AddressMode addressModeU = AddressMode::Repeat;
        AddressMode addressModeV = AddressMode::Repeat;
        AddressMode addressModeW = AddressMode::Repeat;
    };

    friend class MeshInstance;

private:
    Sampler::Filter getSamplerFilter(int filter);
    Sampler::AddressMode getAddressMode(int mode);

public:
    // ====================== material data (public) ========================================

    // used to identify this material.
    Util::String name;

    // a hash of the material name; used as a unique id
    uint32_t materialId;

    MaterialBlock block;
    Sampler sampler;

    // the paths for all textures. Empty paths signify that this texture isn't used
    Util::String texturePaths[TextureType::Count];

    // the material pipeline to use
    MaterialPipeline pipeline = MaterialPipeline::None;

    bool doubleSided = false;

private:
    // ====================== material data (private) ========================================

    // used to find the texture group in the list
    size_t bufferIndex;
};

} // namespace OmegaEngine
