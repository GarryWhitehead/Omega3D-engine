#pragma once
#include <vector>

#include "Rendering/RenderInterface.h"

#include "Engine/Omega_Config.h"

namespace OmegaEngine
{
	// forward class declerations
	struct RenderConfig;
	class RenderInterface;

	class RenderManager
	{

	public:

		RenderManager(RenderConfig& config);
		~RenderManager();

		std::unique_ptr<RenderInterface>& getInterface()
		{
			return render_interface;
		}

	private:

		RenderConfig renderConfig;

		std::unique_ptr<RenderInterface> render_interface;
	};

}