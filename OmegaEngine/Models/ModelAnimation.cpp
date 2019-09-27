#include "ModelAnimation.h"

#include "Models/Formats/GltfModel.h"

#include "utility/Logger.h"

namespace OmegaEngine
{

bool ModelAnimation::prepare(cgltf_animation& anim, GltfModel& model)
{
    name = anim.name;

	// get channel data
    std::vector<cgltf_animation_sampler*> animSamplers;
	uint32_t channelIndex = 0;
    for (size_t i = 0; i < anim.channels_count; ++i)
	{
		ModelAnimation::Channel channel;
        
		channel.pathType = anim.channels[i].target_path;
        // process the samplers below.....
        animSamplers.emplace_back(anim.channels[i].sampler);
        
		// set animation flag on relevant node
	//	auto node = model->getNode(anim.channels[i].target_node->);
	//	assert(node != nullptr);
	//	node->setAnimationIndex(index, channelIndex++);

		channels.emplace_back(channel);
	}

	// get sampler data
	for (cgltf_animation_sampler* animSampler : animSamplers)
	{
		ModelAnimation::Sampler samplerInfo;
		samplerInfo.interpolation = animSampler->interpolation;
        
        // ====== inputs =====
        {
            const cgltf_accessor* accessor = animSampler->input;
            uint8_t* base = static_cast<uint8_t*>(accessor->buffer_view->buffer->data);
            
            // use the stride as a sanity check to make sure we have a matrix
            size_t stride = accessor->buffer_view->stride;
            if (!stride)
            {
                stride = accessor->stride;
            }
            assert(stride);
            assert(stride == 16);

            // only supporting floats at the moment. This can be expaned on if the need arises...
            switch (accessor->component_type)
            {
            case cgltf_component_type_r_32f:
            {
                float* buffer = new float[accessor->count];
                memcpy((void*)buffer, base,
                       accessor->count * sizeof(float));

                for (uint32_t i = 0; i < accessor->count; ++i)
                {
                    samplerInfo.timeStamps.emplace_back(buffer[i]);
                }
                delete[] buffer;
                break;
            }
            default:
                LOGGER_ERROR("Unsupported component type used for time accessor.");
            }

            // time and end points
            for (auto input : samplerInfo.timeStamps)
            {
                if (input < modelAnim->start)
                {
                    modelAnim->start = input;
                }
                if (input > modelAnim->end)
                {
                    modelAnim->end = input;
                }
            }
        }
		// ====== outputs ========
        {
            const cgltf_accessor* accessor = animSampler->output;
            uint8_t* base = static_cast<uint8_t*>(accessor->buffer_view->buffer->data);
            
            // use the stride as a sanity check to make sure we have a matrix
            size_t stride = accessor->buffer_view->stride;
            if (!stride)
            {
                stride = accessor->stride;
            }
            assert(stride);
            assert(stride == 16);

            // again, for now, only supporting floats
            switch (accessor->component_type)
            {
            case cgltf_component_type_r_32f:
            {
                // all types will be converted to vec4 for ease of use
                switch (trsAccessor.type)
                {
                case TINYGLTF_TYPE_VEC3:
                {
                    OEMaths::vec3f* buffer = reinterpret_cast<OEMaths::vec3f*>(
                        &trsBuffer.data[trsAccessor.byteOffset + trsBufferView.byteOffset]);
                    for (uint32_t i = 0; i < trsAccessor.count; ++i)
                    {
                        samplerInfo.outputs.push_back(OEMaths::vec4f(buffer[i], 0.0f));
                    }
                    break;
                }
                case TINYGLTF_TYPE_VEC4:
                {
                    samplerInfo.outputs.resize(trsAccessor.count);
                    memcpy(samplerInfo.outputs.data(), &trsBuffer.data[trsAccessor.byteOffset + trsBufferView.byteOffset],
                           trsAccessor.count * sizeof(OEMaths::vec4f));
                    break;
                }
                default:
                    LOGGER_ERROR("Unsupported component type used for TRS accessor.");
                }
            }
            }
        }
        
		modelAnim->samplers.emplace_back(samplerInfo);
	}
}

}
