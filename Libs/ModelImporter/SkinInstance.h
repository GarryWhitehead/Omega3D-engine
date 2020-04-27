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

#include "utility/CString.h"

#include "cgltf/cgltf.h"

#include <vector>
#include <unordered_map>

namespace OmegaEngine
{
class NodeInstance;
struct NodeInfo;

class SkinInstance
{
public:

	SkinInstance() 
	{
	}

	~SkinInstance()
	{
	}
    
    /**
     * @brief Prepare a skin from a cgltf skin struct. Links joints with nodes in the
     * hierachy.
     * @param skin The cgltf skin to extract data from
     * @param A list of nodes were the 'name' variable has been updated to use a id
     */
	bool prepare(cgltf_skin& skin, NodeInstance& node);

    // =========================== skinning data (public) =============================================

    Util::String name;
    
    // links the bone node name with the inverse transform
    std::vector<OEMaths::mat4f> invBindMatrices;
    
    // a list of joints - poiints to the node in the skeleon hierachy which will be transformed
    std::vector<NodeInfo*> jointNodes;
    
    // a pointer to the the root of the skeleton. The spec states that
    // the mdoel doesn't need to specify this - thus will be null if this is the case.
    NodeInfo* skeletonRoot = nullptr;
    
};

} // namespace OmegaEngine
