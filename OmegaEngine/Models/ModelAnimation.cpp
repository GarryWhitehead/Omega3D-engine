#include "ModelAnimation.h"
#include "Models/GltfModel.h"
#include "Utility/Logger.h"

namespace OmegaEngine
{

ModelAnimation::ModelAnimation()
{
}

ModelAnimation::~ModelAnimation()
{
}

void ModelAnimation::extractAnimationData(tinygltf::Model &gltfModel, tinygltf::Animation &anim,
                                          std::unique_ptr<GltfModel::Model> &model,
                                          const uint32_t index)
{
	name = anim.name.c_str();

	// get channel data
	uint32_t channelIndex = 0;
	for (auto &source : anim.channels)
	{
		Channel channel;

		channel.pathType = source.target_path;
		channel.samplerIndex = source.sampler;

		// set animation flag on relevant node
		auto node = model->getNode(source.target_node);
		assert(node != nullptr);
		node->setAnimationIndex(index, channelIndex++);

		channels.push_back(channel);
	}

	// get sampler data
	for (auto &sampler : anim.samplers)
	{
		Sampler samplerInfo;
		samplerInfo.interpolation = sampler.interpolation;

		tinygltf::Accessor timeAccessor = gltfModel.accessors[sampler.input];
		tinygltf::BufferView timeBufferView = gltfModel.bufferViews[timeAccessor.bufferView];
		tinygltf::Buffer timeBuffer = gltfModel.buffers[timeBufferView.buffer];

		// only supporting floats at the moment. This can be expaned on if the need arises...
		switch (timeAccessor.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_FLOAT:
		{
			float *buffer = new float[timeAccessor.count];
			memcpy(buffer, &timeBuffer.data[timeAccessor.byteOffset + timeBufferView.byteOffset],
			       timeAccessor.count * sizeof(float));

			for (uint32_t i = 0; i < timeAccessor.count; ++i)
			{
				samplerInfo.timeStamps.emplace_back(buffer[i]);
			}
			delete buffer;
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

		// get TRS data
		tinygltf::Accessor trsAccessor = gltfModel.accessors[sampler.output];
		tinygltf::BufferView trsBufferView = gltfModel.bufferViews[trsAccessor.bufferView];
		tinygltf::Buffer trsBuffer = gltfModel.buffers[trsBufferView.buffer];

		// again, for now, only supporting floats
		switch (trsAccessor.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_FLOAT:
		{
			// all types will be converted to vec4 for ease of use
			switch (trsAccessor.type)
			{
			case TINYGLTF_TYPE_VEC3:
			{
				OEMaths::vec3f *buffer = reinterpret_cast<OEMaths::vec3f *>(
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
				memcpy(samplerInfo.outputs.data(),
				       &trsBuffer.data[trsAccessor.byteOffset + trsBufferView.byteOffset],
				       trsAccessor.count * sizeof(OEMaths::vec4f));
				break;
			}
			default:
				LOGGER_ERROR("Unsupported component type used for TRS accessor.");
			}
		}
		}

		samplers.emplace_back(samplerInfo);
	}
}
} // namespace OmegaEngine
