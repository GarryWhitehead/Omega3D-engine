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
	uint32_t AnimationManager::Sampler::indexFromTime(double time)
	{
		uint32_t index = 0;
		uint32_t timestampCount = static_cast<uint32_t>(timeStamps.size());
		if (timestampCount <= 1 || time <= timeStamps.front()) 
		{
			index = 0;
		}
		else if (time >= timeStamps.back()) 
		{
			index = timestampCount - 1;
		}
		else 
		{
			uint32_t endTime = 0;
			while (time > timeStamps[endTime]) 
			{
				++endTime;
			}
			index = endTime - 1;
		}
		return index;
	}

	float AnimationManager::Sampler::getPhase(double time)
	{
		double phase = 0.0;
		uint32_t timestampCount = static_cast<uint32_t>(timeStamps.size());
		if (timestampCount <= 1 || time <= timeStamps.front()) 
		{
			phase = 0.0f;
		}
		else if (time >= timeStamps.back()) 
		{
			phase = 1.0f;
		}
		else 
		{
			uint32_t endTime = 0;
			while (time > timeStamps[endTime]) 
			{
				++endTime;
			}

			uint32_t denom = (timeStamps[endTime] - timeStamps[endTime - 1]);
			denom = denom == 0 ? 1 : denom;
			phase = (time - timeStamps[endTime - 1]) / denom;
		}
		return static_cast<float>(phase);
	}

	void AnimationManager::addGltfAnimation(tinygltf::Model& model, std::unordered_map<uint32_t, Object>& linearisedObjects)
	{

		for (tinygltf::Animation& anim : model.animations) 
		{
			AnimationInfo animInfo;
			animInfo.name = anim.name.c_str();

			// get channel data
			for (auto& source : anim.channels) 
			{
				Channel channel;

				if (source.target_path == "rotation") 
				{
					channel.pathType = Channel::PathType::Rotation;
				}
				if (source.target_path == "scale") 
				{
					channel.pathType = Channel::PathType::Scale;
				}
				if (source.target_path == "translation") 
				{
					channel.pathType = Channel::PathType::Translation;
				}
				if (source.target_path == "weights") 
				{
					LOGGER_INFO("Channel path type weights not yet supported.");
					continue;
				}

				channel.samplerIndex = source.sampler;

				// reference to which object mesh this animation targets - note: this needs thinking about as at the moment we could be pointing to an object
				// that has been destroyed. TODO: first instance add a function that will iterate through, find and remove a object potentially driven by an event
				channel.object = linearisedObjects[source.target_node];
				animInfo.channels.push_back(channel);
			}

			// get sampler data
			for (auto& sampler : anim.samplers) 
			{
				Sampler samplerInfo;

				if (sampler.interpolation == "LINEAR") 
				{
					samplerInfo.interpolationType = Sampler::InerpolationType::Linear;
				}
				if (sampler.interpolation == "STEP") 
				{
					samplerInfo.interpolationType = Sampler::InerpolationType::Step;
				}
				if (sampler.interpolation == "CUBICSPLINE") 
				{
					samplerInfo.interpolationType = Sampler::InerpolationType::CubicSpline;
				}

				tinygltf::Accessor timeAccessor = model.accessors[sampler.input];
				tinygltf::BufferView timeBufferView = model.bufferViews[timeAccessor.bufferView];
				tinygltf::Buffer timeBuffer = model.buffers[timeBufferView.buffer];

				// only supporting floats at the moment. This can be expaned on if the need arises...
				switch (timeAccessor.componentType) 
				{
					case TINYGLTF_COMPONENT_TYPE_FLOAT: 
					{
						float* buffer = new float[timeAccessor.count];
						memcpy(buffer, &timeBuffer.data[timeAccessor.byteOffset + timeBufferView.byteOffset], timeAccessor.count * sizeof(float));

						for (uint32_t i = 0; i < timeAccessor.count; ++i) 
						{
							samplerInfo.timeStamps.push_back(buffer[i]);
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
					if (input < animInfo.start) 
					{
						animInfo.start = input;
					}
					if (input > animInfo.end) 
					{
						animInfo.end = input;
					}
				}

				// get TRS data
				tinygltf::Accessor trsAccessor = model.accessors[sampler.output];
				tinygltf::BufferView trsBufferView = model.bufferViews[trsAccessor.bufferView];
				tinygltf::Buffer trsBuffer = model.buffers[trsBufferView.buffer];

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
							OEMaths::vec3f* buffer = reinterpret_cast<OEMaths::vec3f*>(&trsBuffer.data[trsAccessor.byteOffset + trsBufferView.byteOffset]);
							for (uint32_t i = 0; i < trsAccessor.count; ++i) 
							{
								samplerInfo.outputs.push_back(OEMaths::vec4f(buffer[i], 0.0f));
							}
							break;
						}
						case TINYGLTF_TYPE_VEC4: 
						{
							samplerInfo.outputs.resize(trsAccessor.count);
							memcpy(samplerInfo.outputs.data(), &trsBuffer.data[trsAccessor.byteOffset + trsBufferView.byteOffset], trsAccessor.count * sizeof(OEMaths::vec4f));
							break;
						}
						default:
							LOGGER_ERROR("Unsupported component type used for TRS accessor.");
					}
				}
				}

				animInfo.samplers.push_back(samplerInfo);
			}

			animationBuffer.push_back(animInfo);
		} 
	}

	void AnimationManager::updateAnimation(double time, double dt, TransformManager& transformManager)
	{
		double timeSecs = time / 1000000000;

		for (auto& anim : animationBuffer) 
		{
			float animTime = std::fmod(timeSecs - anim.start, anim.end);

			// go through each target an, caluclate the animation transform and update on the transform manager side
			for (auto& channel : anim.channels) 
			{
				Object obj = channel.object;
				Sampler& sampler = anim.samplers[channel.samplerIndex];

				uint32_t timeIndex = sampler.indexFromTime(animTime);
				float phase = sampler.getPhase(animTime);

				switch (channel.pathType) 
				{
				case Channel::PathType::Translation: 
				{
					OEMaths::vec4f trans;
					trans.mix(sampler.outputs[timeIndex], sampler.outputs[timeIndex + 1], phase);
					transformManager.updateObjectTranslation(obj, trans);
					break;
				}
				case Channel::PathType::Scale: 
				{
					OEMaths::vec4f scale;
					scale.mix(sampler.outputs[timeIndex], sampler.outputs[timeIndex + 1], phase);
					transformManager.updateObjectScale(obj, scale);
					break;
				}
				case Channel::PathType::Rotation: 
				{
					OEMaths::quatf quat1 {
						sampler.outputs[timeIndex].getX(),
						sampler.outputs[timeIndex].getY(),
						sampler.outputs[timeIndex].getZ(),
						sampler.outputs[timeIndex].getW(),
					};

					OEMaths::quatf quat2 {
						sampler.outputs[timeIndex + 1].getX(),
						sampler.outputs[timeIndex + 1].getY(),
						sampler.outputs[timeIndex + 1].getZ(),
						sampler.outputs[timeIndex + 1].getW()
					};

					// TODO: add cubic and step interpolation
					OEMaths::quatf rot;
					rot.linearMix(quat1, quat2, phase);
					rot.normalise();
					transformManager.updateObjectRotation(obj, rot);
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
