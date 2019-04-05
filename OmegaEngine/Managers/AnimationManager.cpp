#include "AnimationManager.h"

#include "Managers/TransformManager.h"
#include "Objects/ObjectManager.h"
#include "Utility/logger.h"
#include "Objects/Object.h"

namespace OmegaEngine
{

	AnimationManager::AnimationManager()
	{
	}


	AnimationManager::~AnimationManager()
	{
	}

	// channel functions
	uint32_t AnimationManager::Sampler::index_from_time(double time)
	{
		uint32_t index = 0;
		uint32_t timestamp_count = static_cast<uint32_t>(time_stamps.size());
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
			index = end_time;
		}
		return index;
	}

	float AnimationManager::Sampler::get_phase(double time)
	{
		double phase = 0.0;
		uint32_t timestamp_count = static_cast<uint32_t>(time_stamps.size());
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
			uint32_t index = end_time;

			uint32_t denom = (time_stamps[end_time + 1] - time_stamps[index]);
			denom = denom == 0 ? 1 : denom;
			phase = (time - time_stamps[index]) / denom;
		}
		return static_cast<float>(phase);
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
						OEMaths::vec3f* buffer = reinterpret_cast<OEMaths::vec3f*>(&trsBuffer.data[trsAccessor.byteOffset + trsBufferView.byteOffset]);
						for (uint32_t i = 0; i < trsAccessor.count; ++i) {
							samplerInfo.outputs.push_back(OEMaths::vec4f(buffer[i], 1.0f));
						}
						break;
					}
					case TINYGLTF_TYPE_VEC4: {
						samplerInfo.outputs.resize(trsAccessor.count);
						memcpy(samplerInfo.outputs.data(), &trsBuffer.data[trsAccessor.byteOffset + trsBufferView.byteOffset], trsAccessor.count * sizeof(OEMaths::vec4f));
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

	void AnimationManager::update_anim(double time, TransformManager& transform_man)
	{
		double time_secs = time / 1000000000;

		for (auto& anim : animationBuffer) {

			// go through each target an, caluclate the animation transform and update on the transform manager side
			for (auto& channel : anim.channels) {

				Object obj = channel.object;
				Sampler& sampler = anim.samplers[channel.samplerIndex];

				float anim_time = std::fmod(time_secs - anim.start, sampler.time_stamps.back());

				uint32_t time_index = sampler.index_from_time(time_secs);
				float phase = sampler.get_phase(time_secs);
				//printf("phase = %f     index = %i\n", phase, time_index);
				switch (channel.pathType) {
				case Channel::PathType::Translation: {
					OEMaths::vec4f trans = OEMaths::mix_vec4(sampler.outputs[time_index], sampler.outputs[time_index + 1], phase);
					transform_man.update_obj_translation(obj, trans);
					break;
				}
				case Channel::PathType::Scale: {
					OEMaths::vec4f scale = OEMaths::mix_vec4(sampler.outputs[time_index], sampler.outputs[time_index + 1], phase);
					transform_man.update_obj_scale(obj, scale);
					break;
				}
				case Channel::PathType::Rotation: {
					OEMaths::quatf quat1 {
						sampler.outputs[time_index].x,
						sampler.outputs[time_index].y,
						sampler.outputs[time_index].z,
						sampler.outputs[time_index].w,
					};

					OEMaths::quatf quat2 {
						sampler.outputs[time_index + 1].x,
						sampler.outputs[time_index + 1].y,
						sampler.outputs[time_index + 1].z,
						sampler.outputs[time_index + 1].w
					};

					// TODO: add cubic and step interpolation
					OEMaths::quatf rot = OEMaths::normalise_quat(OEMaths::linear_mix_quat(quat1, quat2, phase));
					transform_man.update_obj_rotation(obj, rot);
					break;
				}
				case Channel::PathType::CubicScale:
					// TODO
					break;
				case Channel::PathType::CublicTranslation:
					// TODO
					break;
				default:
					LOGGER_ERROR("Invalid animation pathtype encountered.");
				}
			}
			
		}
	}

	
}
