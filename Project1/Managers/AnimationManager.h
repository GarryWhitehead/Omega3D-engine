#pragma once

#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"

#include "tiny_gltf.h"

namespace OmegaEngine
{

	class AnimationManager
	{

	public:

		AnimationManager();
		~AnimationManager();

	private:

		std::unordered_map<uint32_t, std::vector<AnimationInfo> > animationBuffer;
		std::unordered_map<uint32_t, std::vector<SkinInfo> >skinBuffer;
	};

}
