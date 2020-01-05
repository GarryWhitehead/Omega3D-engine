#pragma once

#include "OEMaths/OEMaths.h"

#include "Models/NodeInstance.h"

#include "utility/CString.h"

#include "cgltf/cgltf.h"

#include <vector>
#include <unordered_map>

namespace OmegaEngine
{
    
class SkinInstance
{
public:

	SkinInstance() 
	{
	}

	~SkinInstance()
	{
	}
	 
	// copying and moving allowed
	SkinInstance(const SkinInstance&) = default;
	SkinInstance& operator=(const SkinInstance&) = default;
	SkinInstance(SkinInstance&&) = default;
	SkinInstance& operator=(SkinInstance&&) = default;
    
    /**
     * @brief Prepare a skin from a cgltf skin struct. Links joints with nodes in the
     * hierachy.
     * @param skin The cgltf skin to extract data from
     * @param A list of nodes were the 'name' variable has been updated to use a id
     */
	bool prepare(cgltf_skin& skin, NodeInstance& node);

	friend class TransformManager;

private:
    
    struct Skin
    {
        Util::String name;
        
        // links the bone node name with the inverse transform
        std::vector<OEMaths::mat4f> invBindMatrices;
        
        // a list of joints - poiints to the node in the skeleon hierachy which will be transformed
        std::vector<NodeInstance::NodeInfo*> jointNodes;
        
        // a pointer to the the root of the skeleton. The spec states that
        // the mdoel doesn't need to specify this - thus will be null if this is the case.
        NodeInstance::NodeInfo* skeletonRoot = nullptr;
    };
    
    std::vector<Skin> skins;
};

} // namespace OmegaEngine
