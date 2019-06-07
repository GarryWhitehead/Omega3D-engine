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
