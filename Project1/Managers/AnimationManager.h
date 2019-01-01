#pragma once

#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"

#include "ComponentInterface/ComponentManagerBase.h"
#include "tiny_gltf.h"

namespace OmegaEngine
{
	// forward declerations
	class Object;

	class AnimationManager : public ComponentManagerBase
	{

	public:

		// skins
		struct SkinInfo
		{
			const char* name;
			uint32_t skeletonIndex;

			std::vector<uint32_t> joints;
			std::vector<OEMaths::mat4f> invBindMatrices;
		};


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

		void addGltfAnimation(tinygltf::Model& model, Object& obj);
		void addGltfSkin(tinygltf::Model& model);

	private:

		std::vector<AnimationInfo> animationBuffer;
		std::vector<SkinInfo> skinBuffer;
	};

}
