#include "ModelSkin.h"

#include "Gltf/GltfModel.h"

#include "utility/Logger.h"

namespace OmegaEngine
{

bool ModelSkin::prepare(cgltf_skin & skin)
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

	std::vector<OEMaths::mat4f> matrices(accessor->count);
	memcpy(matrices.data(), base, accessor->count * sizeof(OEMaths::mat4f));

	if (invBindMatrices.size() != skin.joints_count)
	{
		LOGGER_ERROR("The inverse bind matrices and joint sizes don't match.\n");
		return false;
	}

	// and now for bones
	for (size_t i = 0; i < skin.joints_count; ++i)
	{
		cgltf_node* boneNode = skin.joints[i];

		Util::String nodeName = getNodeName(boneNode);
		invBindMatrices.emplace(nodeName, matrices[i]);
	}

	skeletonRoot = getNodeName(skin.skeleton);
}

}