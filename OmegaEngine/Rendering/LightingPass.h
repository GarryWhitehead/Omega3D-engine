#pragma once

#include "RenderGraph/Resources.h"

namespace OmegaEngine
{

// forward declerations
class RenderGraph;

class LightingPass
{
public:

	struct LPassInfo
	{
		ResourceHandle output;
	};

	LightingPass()
	{
	}

	// no copying
	LightingPass(const LightingPass&) = delete;
	LightingPass& operator=(const LightingPass&) = delete;

	void init();

private:

	RenderGraph* rGraph = nullptr;

	LPassInfo passInfo;
};
}    // namespace OmegaEngine