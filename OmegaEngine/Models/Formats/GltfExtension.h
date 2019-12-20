#pragma once

#include "utility/CString.h"

#include "OEMaths/OEMaths.h"

#include "cgltf/cgltf.h"

#include <unordered_map>

namespace OmegaEngine
{

// <extension name, value (as string)>
using ExtensionData = std::unordered_map<const char*, Util::String>;

class GltfExtension
{
public:
    
    // utility functions for dealing with gltf json data
    bool prepare(const cgltf_extras& extras, cgltf_data& data);
    
    /// token to string converion functions
    static OEMaths::vec3f tokenToVec3(Util::String str);
    
    Util::String find(Util::String ext);
    
private:
    
    std::unordered_map<const char*, Util::String> extensions;
};

}
