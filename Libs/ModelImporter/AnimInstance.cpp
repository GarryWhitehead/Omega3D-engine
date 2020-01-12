#include "AnimInstance.h"

#include "Formats/GltfModel.h"

#include "NodeInstance.h"

#include "utility/Logger.h"

namespace OmegaEngine
{

Util::String AnimInstance::cgltfSamplerToStr(const cgltf_interpolation_type type)
{
    Util::String result;
    switch(type)
    {
        case cgltf_interpolation_type_linear:
            result = "linear";
            break;
        case cgltf_interpolation_type_step:
            result = "step";
            break;
        case cgltf_interpolation_type_cubic_spline:
            result = "spline";
            break;
    }
    return result;
}

Util::String AnimInstance::cgltfPathTypeToStr(const cgltf_animation_path_type type)
{
    Util::String result;
    switch(type)
    {
        case cgltf_animation_path_type_scale:
            result = "Scale";
            break;
        case cgltf_animation_path_type_weights:
            result = "Weights";
            break;
        case cgltf_animation_path_type_rotation:
            result = "rotation";
            break;
        case cgltf_animation_path_type_translation:
            result = "translation";
            break;
    }
    return result;
}

bool AnimInstance::prepare(cgltf_animation& anim, GltfModel& model)
{
	name = anim.name;

	// get channel data
	std::vector<cgltf_animation_sampler*> animSamplers;
	uint32_t channelIndex = 0;

	for (size_t i = 0; i < anim.channels_count; ++i)
	{
		AnimInstance::Channel channel;
        
		channel.pathType = cgltfPathTypeToStr(anim.channels[i].target_path);
		// process the samplers below.....
		animSamplers.emplace_back(anim.channels[i].sampler);

		// set animation flag on relevant node
		cgltf_node* animNode = anim.channels[i].target_node;
        NodeInfo* foundNode = model.getNode(Util::String(animNode->name));
		if (!foundNode)
		{
			LOGGER_ERROR("Unable to find animation node %s within the node hierachy.\n", animNode->name);
			return false;
		}

		foundNode->setChannelIdx(channelIndex++);
		channels.emplace_back(channel);
	}

	// get sampler data
	for (cgltf_animation_sampler* animSampler : animSamplers)
	{
		AnimInstance::Sampler samplerInfo;
		samplerInfo.interpolation = cgltfSamplerToStr(animSampler->interpolation);

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
				memcpy((void*)buffer, base, accessor->count * sizeof(float));

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
				if (input < start)
				{
					start = input;
				}
				if (input > end)
				{
					end = input;
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
				switch (accessor->type)
				{
				case cgltf_type_vec3:
				{
					OEMaths::vec3f* buffer = reinterpret_cast<OEMaths::vec3f*>(base);
					for (uint32_t i = 0; i < accessor->count; ++i)
					{
						samplerInfo.outputs.push_back(OEMaths::vec4f(buffer[i], 0.0f));
					}
					break;
				}
				case cgltf_type_vec4:
				{
					samplerInfo.outputs.resize(accessor->count);
					memcpy(samplerInfo.outputs.data(), base, accessor->count * sizeof(OEMaths::vec4f));
					break;
				}
                case cgltf_type_vec2:
				default:
					LOGGER_ERROR("Unsupported component type used for animation output.");
				}
			}
            // these are possible formats supported by gltf, but we only support 32bit floats at present
            case cgltf_component_type_invalid:
            case cgltf_component_type_r_8:
            case cgltf_component_type_r_8u:
            case cgltf_component_type_r_16:
            case cgltf_component_type_r_16u:
            case cgltf_component_type_r_32u:
            default:
                LOGGER_ERROR("Unsupported component type used for animation output.");
			}
		}

		samplers.emplace_back(samplerInfo);
	}
    return true;
}

}    // namespace OmegaEngine
