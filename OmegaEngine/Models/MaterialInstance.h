#pragma once

#include "OEMaths/OEMaths.h"

#include "utility/String.h"
#include "utility/BitsetEnum.h"

#include "VulkanAPI/Managers/ProgramManager.h"

#include "cgltf/cgltf.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"

#include <cstdint>
#include <cassert>

namespace OmegaEngine
{

enum TextureType : size_t
{
	BaseColour,
	Emissive,
	MetallicRoughness,
	Normal,
	Occlusion,
	Count
};

class MaterialInstance
{
public:
    
	MaterialInstance();
	~MaterialInstance();

	// copyable and moveable
	MaterialInstance(const MaterialInstance&) = default;
	MaterialInstance& operator=(const MaterialInstance&) = default;
	MaterialInstance(MaterialInstance&&) = default;
	MaterialInstance& operator=(MaterialInstance&&) = default;

	Util::String getTextureUri(cgltf_texture_view& view, const MaterialInstance::TextureVariant bit);

	bool prepare(cgltf_material& mat);

	bool prepare(aiMaterial* mat);

	// helper functions
	Util::String getName()
	{
		assert(!name.empty());    // sanity check!
		return name;
	}

	friend class RenderableManager;
	friend class GBufferFillPass;

private:
    
    /**
     * @brief The main attributes for this material. The data layout reflects the push block used by the
     * shader - so any changes must be adopted in both places.
     */
    struct MaterialBlock
    {
        OEMaths::vec3f emissive = OEMaths::vec3f{ 1.0f };
        OEMaths::vec4f baseColour = OEMaths::vec4f{ 1.0f };
        OEMaths::vec4f diffuse = OEMaths::vec4f{ 1.0f };
        OEMaths::vec3f specular = OEMaths::vec3f{ 0.0f };

        float specularGlossiness = 1.0f;
        float roughness = 1.0f;
        float metallic = 1.0f;
        
        Util::String mask;
        float alphaMaskCutOff = 1.0f;
    };
    
private:
    
	// used to identify this material. 
	Util::String name;

	// used to find the texture group in the list
	size_t bufferIndex;

    MaterialBlock block;
    
	// the paths for all textures. Empty paths signify that this texture isn't used
	Util::String texturePaths[TextureType::Count];

	// if using specular glossiness then color and metallic/roughness texture indicies will be automatically changed for this workflow
	bool usingSpecularGlossiness = false;

	bool doubleSided = false;

};

}    // namespace OmegaEngine
