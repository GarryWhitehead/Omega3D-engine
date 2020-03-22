#include "AnimationManager.h"

#include "Core/engine.h"

#include "Components/TransformManager.h"

#include "ModelImporter/AnimInstance.h"

#include "utility/Logger.h"

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
		index = timestampCount - 2;
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

void AnimationManager::addAnimation(std::unique_ptr<AnimInstance>& animation)
{
	AnimationInfo animInfo;

	// copy the samplers
	for (auto& sampler : animation->samplers)
	{
		Sampler newSampler;

		if (sampler.interpolation.compare("Linear"))
		{
			newSampler.interpolationType = Sampler::InterpolationType::Linear;
		}
		else if (sampler.interpolation.compare("Step"))
		{
			newSampler.interpolationType = Sampler::InterpolationType::Step;
		}
		else if (sampler.interpolation.compare("CubicSpline"))
		{
			newSampler.interpolationType = Sampler::InterpolationType::CubicSpline;
		}
		else
		{
			LOGGER_INFO("Note: Unsupported sampler interpolation type requested.");
		}

		// straight copy
		newSampler.timeStamps.resize(sampler.timeStamps.size());
		memcpy(newSampler.timeStamps.data(), sampler.timeStamps.data(), newSampler.timeStamps.size() * sizeof(float));

		newSampler.outputs.resize(sampler.outputs.size());
		memcpy(newSampler.outputs.data(), sampler.outputs.data(), newSampler.outputs.size() * sizeof(OEMaths::vec4f));

		animInfo.samplers.emplace_back(newSampler);
	}

	for (auto& channel : animation->channels)
	{
		Channel newChannel;

		if (channel.pathType.compare("rotation"))
		{
			newChannel.type = Channel::PathType::Rotation;
		}
		if (channel.pathType.compare("scale"))
		{
			newChannel.type = Channel::PathType::Scale;
		}
		if (channel.pathType.compare("translation"))
		{
			newChannel.type = Channel::PathType::Translation;
		}
		if (channel.pathType.compare("weights"))
		{
			LOGGER_INFO("Channel path type weights not yet supported.");
			continue;
		}

		newChannel.samplerIndex = channel.samplerIndex;

		// the rest of the channel will be set later
		animInfo.channels.emplace_back(newChannel);
	}

	// start and end times for this animation
	animInfo.start = animation->getStartTime();
	animInfo.end = animation->getEndTime();

	animations.emplace_back(animInfo);
}

void AnimationManager::addAnimation(size_t channelIndex, size_t bufferIndex, OEObject& object)
{
	Channel& channel = animations[bufferIndex].channels[channelIndex];
	// link object with animation channel
	channel.object = &object;
}

void AnimationManager::update(double time, OEEngine& engine)
{
	auto& transManager = engine.getTransManager();

	double timeSecs = time / 1000000000;

	for (auto& anim : animations)
	{
		float animTime = std::fmod(timeSecs - anim.start, anim.end);

		// go through each target and caluclate the animation transform and update on the transform manager side
		for (auto& channel : anim.channels)
		{
			OEObject* obj = channel.object;
			Sampler& sampler = anim.samplers[channel.samplerIndex];

			uint32_t timeIndex = sampler.indexFromTime(animTime);
			float phase = sampler.getPhase(animTime);

			switch (channel.type)
			{
			case Channel::PathType::Translation:
			{
				OEMaths::vec4f trans = OEMaths::mix(sampler.outputs[timeIndex], sampler.outputs[timeIndex + 1], phase);
				transManager.updateObjectTranslation(*obj, trans.xyz);
				break;
			}
			case Channel::PathType::Scale:
			{
				OEMaths::vec4f scale = OEMaths::mix(sampler.outputs[timeIndex], sampler.outputs[timeIndex + 1], phase);
				transManager.updateObjectScale(*obj, scale.xyz);
				break;
			}
			case Channel::PathType::Rotation:
			{
				OEMaths::quatf quat1{
					sampler.outputs[timeIndex].x,
					sampler.outputs[timeIndex].y,
					sampler.outputs[timeIndex].z,
					sampler.outputs[timeIndex].w,
				};

				OEMaths::quatf quat2{ sampler.outputs[timeIndex + 1].x, sampler.outputs[timeIndex + 1].y,
					                  sampler.outputs[timeIndex + 1].z, sampler.outputs[timeIndex + 1].w };

				// TODO: add cubic and step interpolation
                OEMaths::quatf rot = OEMaths::quatf::normalise(OEMaths::quatf::linearMix(quat1, quat2, phase));
				transManager.updateObjectRotation(*obj, rot);
				break;
			}
			case Channel::PathType::CubicScale:
				// TODO
				break;
			case Channel::PathType::CublicTranslation:
				// TODO
				break;
			default:
				LOGGER_INFO("This shouldn't happen! Invalid animation pathtype encountered.");
			}
		}
	}
}

}    // namespace OmegaEngine
