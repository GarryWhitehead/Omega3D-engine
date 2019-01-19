#pragma once
#include "Renderer.h"

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