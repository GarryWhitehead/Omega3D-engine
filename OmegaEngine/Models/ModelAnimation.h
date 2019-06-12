#pragma once
#include "OEMaths/OEMaths.h"
#include "Models/GltfModel.h"
#include "tiny_gltf.h"

namespace OmegaEngine
{
	// forward declerations
	class ModelNode;

	struct Sampler
	{
		std::string interpolation;

		std::vector<float> timeStamps;
		std::vector<OEMaths::vec4f> outputs;
	};

	struct Channel
	{
		std::string  pathType;

		// pointer "got" from a unique ptr - should change
		ModelNode* node;
		uint32_t samplerIndex;
	};

	class ModelAnimation
	{

	public:

		ModelAnimation();
		~ModelAnimation();

		void ModelAnimation::extractAnimationData(tinygltf::Model& gltfModel, tinygltf::Animation& anim, std::unique_ptr<GltfModel::Model>& model, const uint32_t index);

		std::vector<Sampler>& getSamplers()
		{
			return samplers;
		}

		std::vector<Channel>& getChannels()
		{
			return channels;
		}

		uint32_t getStartTime() const
		{
			return start;
		}

		uint32_t getEndTime() const
		{
			return end;
		}

	private:

		const char* name;
		float start = std::numeric_limits<float>::max();
		float end = std::numeric_limits<float>::min();
		std::vector<Sampler> samplers;
		std::vector<Channel> channels;

	};

}

