#pragma once

#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"
#include "DataTypes/object.h"

#include "ComponentInterface/ComponentManagerBase.h"
#include "tiny_gltf.h"

#define MAX_NUM_JOINTS 128

namespace OmegaEngine
{
	// forward declerations
	class Object;
	class TransformManager;
	class ObjectManager;

	class AnimationManager
	{

	public:

		// animation
		struct Sampler
		{
			enum class InerpolationType
			{
				Linear,
				Step,
				CubicSpline
			} interpolationType;


			std::vector<float> time_stamps;
			std::vector<OEMaths::vec4f> outputs;

			uint32_t index_from_time(float time);
			float get_phase(float time);
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

			Object object;
			uint32_t samplerIndex;

		};

		struct AnimationInfo
		{
			const char* name;
			float start;
			float end;
			std::vector<Sampler> samplers;
			std::vector<Channel> channels;

		};

		AnimationManager();
		~AnimationManager();

		void addGltfAnimation(tinygltf::Model& model, std::vector<Object>& linearised_objects);

		void update_anim(double time, std::unique_ptr<TransformManager>& transform_man);

	private:

		std::vector<AnimationInfo> animationBuffer;
	};

}
