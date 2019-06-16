#pragma once
#include "OEMaths/OEMaths.h"
#include "tiny_gltf.h"

#include <memory>

namespace OmegaEngine
{
	// forward declerations
	class ModelNode;
	namespace GltfModel
	{
		struct Model;
	}

	class ModelAnimation
	{

	public:

		struct Sampler
		{
			std::string interpolation;
			std::vector<float> timeStamps;
			std::vector<OEMaths::vec4f> outputs;
		};

		struct Channel
		{
			std::string pathType;
			uint32_t samplerIndex;
		};

		ModelAnimation();
		~ModelAnimation();

		void extractAnimationData(tinygltf::Model& gltfModel, tinygltf::Animation& anim, std::unique_ptr<GltfModel::Model>& model, const uint32_t index);

		std::vector<Sampler>& getSamplers()
		{
			return samplers;
		}

		std::vector<Channel>& getChannels()
		{
			return channels;
		}

		float getStartTime() const
		{
			return start;
		}

		float getEndTime() const
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

