#pragma once
#include "DeferredRenderer.h"

namespace OmegaEngine
{

	struct RenderConfig
	{
		struct General
		{
			RendererType renderer = RendererType::Deferred;

		} general;
	};

}