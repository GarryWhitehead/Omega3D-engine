/* Copyright (c) 2018-2020 Garry Whitehead
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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
