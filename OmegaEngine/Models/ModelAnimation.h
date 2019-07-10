#pragma once
#include "OEMaths/OEMaths.h"

#include <vector>

namespace OmegaEngine
{

struct ModelAnimation
{
	ModelAnimation() = default;
	~ModelAnimation()
	{
	}

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

	const char *name;
	float start = std::numeric_limits<float>::max();
	float end = std::numeric_limits<float>::min();
	std::vector<Sampler> samplers;
	std::vector<Channel> channels;
};

} // namespace OmegaEngine
