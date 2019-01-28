#include "AnimationManager.h"

#include "Managers/TransformManager.h"
#include "ComponentInterface/ObjectManager.h"
#include "Utility/logger.h"
#include "DataTypes/Object.h"

namespace OmegaEngine
{

	AnimationManager::AnimationManager()
	{
	}


	AnimationManager::~AnimationManager()
	{
	}

	// channel functions
	uint32_t AnimationManager::Sampler::index_from_time(float time)
	{
		uint32_t index = 0;
		uint32_t timestamp_count = time_stamps.size();
		if (timestamp_count <= 1 || time < time_stamps.front()) {
			index = 0;
		}
		else if (time >= time_stamps.back()) {
			index = timestamp_count - 2;
		}
		else {
			uint32_t end_time = 0;
			while (time > time_stamps[end_time]) {
				++end_time;
			}
			index = end_time - 1;
		}
		return index;
	}

	float AnimationManager::Sampler::get_phase(float time)
	{
		float phase = 0.0f;
		uint32_t timestamp_count = time_stamps.size();
		if (timestamp_count <= 1 || time < time_stamps.front()) {
			phase = 0.0f;
		}
		else if (time >= time_stamps.back()) {
			phase = 1.0f;
		}
		else {
			uint32_t end_time = 0;
			while (time > time_stamps[end_time]) {
				++end_time;
			}
			uint32_t index = end_time - 1;
			phase = (time - time_stamps[index]) / time_stamps[end_time] - time_stamps[index]);
		}
		return phase;
	}

	void AnimationManager::addGltfAnimation(tinygltf::Model& model, std::vector<Object>& linearised_objects)
	{

		for (tinygltf::Animation& anim : model.animations) {

			AnimationInfo animInfo;
			animInfo.name = anim.name.c_str();

			// get channel data
			for (auto& source : anim.channels) {
				Channel channel;

				if (source.target_path == "rotation") {
					channel.pathType = Channel::PathType::Rotation;
				}
				if (source.target_path == "scale") {
					channel.pathType = Channel::PathType::Scale;
				}
				if (source.target_path == "translation") {
					channel.pathType = Channel::PathType::Translation;
				}
				if (source.target_path == "weights") {
					LOGGER_INFO("Channel path type weights not yet supported.");
					continue;
				}

				channel.samplerIndex = source.sampler;

				// reference to which object mesh this animation targets - note: this needs thinking about as at the moment we could be pointing to an object
				// that has been destroyed. TODO: first instance add a function that will iterate through, find and remove a object potentially driven by an event
				channel.object = linearised_objects[source.target_node];
				animInfo.channels.push_back(channel);
			}

			// get sampler data
			for (auto& sampler : anim.samplers) {
				Sampler samplerInfo;

				if (sampler.interpolation == "LINEAR") {
					samplerInfo.interpolationType = Sampler::InerpolationType::Linear;
				}
				if (sampler.interpolation == "STEP") {
					samplerInfo.interpolationType = Sampler::InerpolationType::Step;
				}
				if (sampler.interpolation == "CUBICSPLINE") {
					samplerInfo.interpolationType = Sampler::InerpolationType::CubicSpline;
				}

				tinygltf::Accessor timeAccessor = model.accessors[sampler.input];
				tinygltf::BufferView timeBufferView = model.bufferViews[timeAccessor.bufferView];
				tinygltf::Buffer timeBuffer = model.buffers[timeBufferView.buffer];

				// only supporting floats at the moment. This can be expaned on if the need arises...
				switch (timeAccessor.componentType) {
				case TINYGLTF_COMPONENT_TYPE_FLOAT: {
					float* buffer = new float[timeAccessor.count];
					memcpy(buffer, &timeBuffer.data[timeAccessor.byteOffset + timeBufferView.byteOffset], timeAccessor.count * sizeof(float));

					for (uint32_t i = 0; i < timeAccessor.count; ++i) {
						samplerInfo.time_stamps.push_back(buffer[i]);
					}
					delete buffer;
					break;
				}
				default:
					LOGGER_ERROR("Unsupported component type used for time accessor.");
					throw std::runtime_error("Unsupported component type whilst parsing gltf file.");
				}

				// time and end points
				for (auto input : samplerInfo.time_stamps) {
					if (input < animInfo.start) {
						animInfo.start = input;
					}
					if (input > animInfo.end) {
						animInfo.end = input;
					}
				}

				// get TRS data
				tinygltf::Accessor trsAccessor = model.accessors[sampler.output];
				tinygltf::BufferView trsBufferView = model.bufferViews[trsAccessor.bufferView];
				tinygltf::Buffer trsBuffer = model.buffers[trsBufferView.buffer];

				// again, for now, only supporting floats
				switch (trsAccessor.componentType) {
				case TINYGLTF_COMPONENT_TYPE_FLOAT: {

					// all types will be converted to vec4 for ease of use
					switch (trsAccessor.type) {
					case TINYGLTF_TYPE_VEC3: {
						OEMaths::vec3f* buffer = new OEMaths::vec3f[trsAccessor.count];
						memcpy(buffer, &trsBuffer.data[trsAccessor.byteOffset + trsBufferView.byteOffset], trsAccessor.count * sizeof(OEMaths::vec3f));
						for (uint32_t i = 0; i < trsAccessor.count; ++i) {
							samplerInfo.outputs.push_back(OEMaths::vec4f(buffer[i], 1.0f));
						}
						delete buffer;
						break;
					}
					case TINYGLTF_TYPE_VEC4: {
						OEMaths::vec4f* buffer = new OEMaths::vec4f[trsAccessor.count];
						memcpy(buffer, &trsBuffer.data[trsAccessor.byteOffset + trsBufferView.byteOffset], trsAccessor.count * sizeof(OEMaths::vec4f));
						for (uint32_t i = 0; i < trsAccessor.count; ++i) {
							samplerInfo.outputs.push_back(buffer[i]);
						}
						delete buffer;
						break;
					}
					default:
						LOGGER_ERROR("Unsupported component type used for TRS accessor.");
						throw std::runtime_error("Unsupported component type whilst parsing gltf file.");
					}
				}
				}

				animInfo.samplers.push_back(samplerInfo);
			}

			animationBuffer.push_back(animInfo);
		} 
	}

	void AnimationManager::update_anim(double time, std::unique_ptr<TransformManager>& transform_man)
	{
		for (auto& anim : animationBuffer) {

			// go through each target an, caluclate the animation transform and update on the transform manager side
			for (auto& channel : anim.channels) {

				Object obj = channel.object;
				Sampler& sampler = anim.samplers[channel.samplerIndex];

				float anim_time = std::fmod(time - anim.start, sampler.time_stamps.back());

				uint32_t time_index = sampler.index_from_time(time);
				float phase = sampler.get_phase(time);

				switch (channel.pathType) {
				case Channel::PathType::Translation: {
					OEMaths::vec4f trans = OEMaths::mix_vec4(sampler.outputs[time_index], sampler.outputs[time_index + 1], phase);
					transform_man->update_obj_translation(obj, trans);
					break;
				}
				case Channel::PathType::Scale: {
					OEMaths::vec4f scale = OEMaths::mix_vec4(sampler.outputs[time_index], sampler.outputs[time_index + 1], phase);
					transform_man->update_obj_scale(obj, scale);
					break;
				}
				case Channel::PathType::Rotation: {
					OEMaths::quatf quat1;
					quat1.x = sampler.outputs[time_index].x;
					quat1.y = sampler.outputs[time_index].y;
					quat1.z = sampler.outputs[time_index].z;
					quat1.w = sampler.outputs[time_index].w;

					OEMaths::quatf quat2;
					quat2.x = sampler.outputs[time_index + 1].x;
					quat2.y = sampler.outputs[time_index + 1].y;
					quat2.z = sampler.outputs[time_index + 1].z;
					quat2.w = sampler.outputs[time_index + 1].w;

					// TODO: add cubic and step interpolation
					OEMaths::quatf rot = OEMaths::linear_mix_quat(quat1, quat2, phase);
					OEMaths::normalise_quat(rot);
					transform_man->update_obj_rotation(obj, rot);
					break;
				}
				case Channel::PathType::CubicScale:
					// TODO
					break;
				case Channel::PathType::CublicTranslation:
					// TODO
					break;
				default:
					LOGGER_ERROR("Unvalid animation pathtype encountered.");
				}
			}
			
		}
	}

	
}
