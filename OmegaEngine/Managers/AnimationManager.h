#pragma once

#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"
#include "ObjectInterface/Object.h"
#include "Managers/ManagerBase.h"

#include <memory>
#include <vector>

#define MAX_NUM_JOINTS 128

namespace OmegaEngine
{
	// forward declerations
	class Object;
	class TransformManager;
	class ObjectManager;
	class ModelAnimation;
	struct AnimationComponent;
	class ComponentInterface;

	class AnimationManager : public ManagerBase
	{

	public:

		// animation
		struct Sampler
		{
			enum class InterpolationType
			{
				Linear,
				Step,
				CubicSpline
			} interpolationType;

			std::vector<float> timeStamps;
			std::vector<OEMaths::vec4f> outputs;

			uint32_t indexFromTime(double time);
			float getPhase(double time);
		};

		struct Channel
		{
			enum class PathType
			{
				Translation,
				Rotation,
				Scale,
				CublicTranslation,
				CubicScale
			} pathType;

			Object* object;
			uint32_t samplerIndex;
		};

		struct AnimationInfo
		{
			const char* name;
			float start = std::numeric_limits<float>::max();
			float end = std::numeric_limits<float>::min();
			std::vector<Sampler> samplers;
			std::vector<Channel> channels;

		};

		AnimationManager();
		~AnimationManager();

		void addComponentToManager(AnimationComponent* component, Object& object);
		void addAnimation(std::unique_ptr<ModelAnimation>& animation);

		void updateFrame(double time, double dt, std::unique_ptr<ObjectManager>& objectManager, ComponentInterface* componentInterface) override;

		uint32_t getBufferOffset() const
		{
			return static_cast<uint32_t>(animations.size());
		}

	private:

		std::vector<AnimationInfo> animations;
	};

}
