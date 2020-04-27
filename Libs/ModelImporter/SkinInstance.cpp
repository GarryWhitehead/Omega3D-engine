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

#include "SkinInstance.h"

#include "NodeInstance.h"

#include "utility/Logger.h"

#include <cstring>

namespace OmegaEngine
{

bool SkinInstance::prepare(cgltf_skin& skin, NodeInstance& node)
{
	// extract the inverse bind matrices
	const cgltf_accessor* accessor = skin.inverse_bind_matrices;
	uint8_t* base = static_cast<uint8_t*>(accessor->buffer_view->buffer->data);  

	// use the stride as a sanity check to make sure we have a matrix
	size_t stride = accessor->buffer_view->stride;                             
	if (!stride)
	{
		stride = accessor->stride;
	}
	assert(stride);
	assert(stride == 16);

	invBindMatrices.reserve(accessor->count);
	memcpy(invBindMatrices.data(), base, accessor->count * sizeof(OEMaths::mat4f));

	if (invBindMatrices.size() != skin.joints_count)
	{
		LOGGER_ERROR("The inverse bind matrices and joint sizes don't match.\n");
		return false;
	}

	// and now for bones
	for (size_t i = 0; i < skin.joints_count; ++i)
	{
		cgltf_node* boneNode = skin.joints[i];
        
        // find the node in the list
        NodeInfo* foundNode = node.getNode(Util::String(boneNode->name));
        if (!foundNode)
        {
            LOGGER_ERROR("Unable to find bone in list of nodes\n");
            return false;
        }
	}
    
    // the model may not have a root for the skeleton. This isn't a requirement by the spec.
    if (skin.skeleton)
    {
        skeletonRoot = node.getNode(Util::String(skin.skeleton->name));
    }
    
    return true;
}

}
