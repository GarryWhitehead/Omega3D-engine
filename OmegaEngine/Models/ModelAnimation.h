#pragma once
#include "OEMaths/OEMaths.h"

#include "utility/String.h"

#include "cgltf/cgltf.h"

#include <vector>

namespace OmegaEngine
{

class ModelAnimation
{
public:
	ModelAnimation() = default;

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

	bool prepare(cgltf_animation& anim, GltfModel& model);


private:
	Util::String name;
	float start = std::numeric_limits<float>::max();
	float end = std::numeric_limits<float>::min();
	std::vector<Sampler> samplers;
	std::vector<Channel> channels;
};

}    // namespace OmegaEngine
