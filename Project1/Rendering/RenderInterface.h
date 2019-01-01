#pragma once

#include "Rendering/RenderableTypes.h"

#include <vector>
#include <memory>

namespace OmegaEngine
{
	// forward decleartions
	class ComponentInterface;
	class Object;

	class RenderInterface
	{

	public:

		RenderInterface();
		~RenderInterface();

		void add_static_mesh(std::unique_ptr<ComponentInterface>& comp_interface, Object& obj);

	private:

		std::vector<RenderableType> renderables;
	};

}

