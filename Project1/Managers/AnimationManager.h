#pragma once

#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"

#include "tiny_gltf.h"

namespace OmegaEngine
{

	class AnimationManager
	{

	public:

		struct SkinInfo
		{
			const char* name;
			uint32_t skeletonIndex;

			std::vector<uint32_t> joints;
			std::vector<OEMaths::mat4f> invBindMatrices;
		};

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

		void parseGltfAnimation(uint32_t spaceId, tinygltf::Model& model);
		void parseGltfSkin(uint32_t spaceId, tinygltf::Model& model);

	private:

		std::unordered_map<uint32_t, std::vector<AnimationInfo> > animationBuffer;
		std::unordered_map<uint32_t, std::vector<SkinInfo> >skinBuffer;
	};

}
