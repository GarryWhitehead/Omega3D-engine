#pragma once
#include "tiny_gltf.h"

namespace OmegaEngine
{
	class Space
	{

	public:

		Space();
		~Space();
		
		bool loadModelData(std::string filename);

	private:

		// Everything model wise needed for this space
		tinygltf::Model model;
	};

}

