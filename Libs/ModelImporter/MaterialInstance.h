#pragma once

#include "OEMaths/OEMaths.h"

#include "utility/CString.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"

#include "cgltf/cgltf.h"

#include <cassert>
#include <cstdint>

namespace OmegaEngine
{
// forward declerations
class MeshInstance;
class GltfExtension;

class MaterialInstance
{
public:
    
	enum TextureType : uint32_t
	{
		BaseColour,
		Emissive,
		MetallicRoughness,
		Normal,
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
		assert(!name.empty());    // sanity check!
		return name;
	}

	/**
     * @brief The main attributes for this material. 
     */
	struct MaterialBlock
	{
		OEMaths::vec4f baseColour = OEMaths::vec4f{ 1.0f };
        OEMaths::vec4f emissive = OEMaths::vec4f{ 1.0f };
		OEMaths::vec4f diffuse = OEMaths::vec4f{ 1.0f };
		OEMaths::vec4f specular = OEMaths::vec4f{ 0.0f };
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

	// if using specular glossiness then color and metallic/roughness texture indicies will be automatically changed for this workflow
	bool usingSpecularGlossiness = false;

	bool doubleSided = false;

private:

	// ====================== material data (private) ========================================

	// used to find the texture group in the list
	size_t bufferIndex;

};

}    // namespace OmegaEngine
