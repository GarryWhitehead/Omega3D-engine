#pragma once
#include "OEMaths/OEMaths.h"

#include "tiny_gltf.h"

namespace OmegaEngine
{
	// forward declerations
	class ModelNode;

	struct Sampler
	{
		enum class InerpolationType
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

		// pointer "got" from a unique ptr - should change
		ModelNode* node;
		uint32_t samplerIndex;

	};

	class ModelAnimation
	{

	public:

		ModelAnimation();
		~ModelAnimation();

		void ModelAnimation::extractAnimationData(tinygltf::Model& gltfModel, tinygltf::Animation& anim, std::unique_ptr<GltfModel::Model>& model);

	private:

		const char* name;
		float start = std::numeric_limits<float>::max();
		float end = std::numeric_limits<float>::min();
		std::vector<Sampler> samplers;
		std::vector<Channel> channels;

	};

}

