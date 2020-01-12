#pragma once
#include "OEMaths/OEMaths.h"

#include "utility/CString.h"

#include "cgltf/cgltf.h"

#include <vector>

namespace OmegaEngine
{
class GltfModel;

class AnimInstance
{
public:
	AnimInstance() = default;

	struct Sampler
	{
		Util::String interpolation;
		std::vector<float> timeStamps;
		std::vector<OEMaths::vec4f> outputs;
	};

	struct Channel
	{
        Util::String pathType;
		uint32_t samplerIndex;
	};

	bool prepare(cgltf_animation& anim, GltfModel& model);

	float getStartTime() const
	{
		return start;
	}

	float getEndTime() const
	{
		return end;
	}

private:
    
    Util::String cgltfSamplerToStr(const cgltf_interpolation_type type);
    Util::String cgltfPathTypeToStr(const cgltf_animation_path_type type);
    
public:

	// =================== animation data (public) ===================================

	Util::String name;
	std::vector<Sampler> samplers;
	std::vector<Channel> channels;

private:

	// =================== animation data (private) ===================================

	float start = std::numeric_limits<float>::max();
	float end = std::numeric_limits<float>::min();
	
};

}    // namespace OmegaEngine
