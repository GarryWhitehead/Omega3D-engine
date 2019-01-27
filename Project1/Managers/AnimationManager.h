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

	class AnimationManager : public ComponentManagerBase
	{

	public:

		// animation
		struct Sampler
		{
			enum class InerpolationType
			{
				Linear,
				Step,
				CubicSpline,
				Count
			} interpolationType;


			std::vector<float> inputs;
			std::vector<OEMaths::vec4f> outputs;
		};

		struct Channel
		{
			enum class PathType
			{
				Translation,
				Rotation,
				Scale,
				Count
			} pathType;

			uint32_t nodeIndex;
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

	private:

		std::vector<AnimationInfo> animationBuffer;
	};

}
