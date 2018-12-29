#pragma once
#include "OEMaths/OEMaths.h"

namespace OmegaEngine
{

	class TransformManager
	{

	public:

		struct TransformData
		{
			std::vector<OEMaths::mat4f> local;
			OEMaths::mat4f world;
		};

		TransformManager();
		~TransformManager();

		void addData(OEMaths::mat4f world, OEMaths::mat4f local);

	private:

		std::vector<TransformData> transformBuffer;
	};

}

