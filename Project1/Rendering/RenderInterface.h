#pragma once

#include "Rendering/RenderableTypes.h"
#include "Vulkan/Device.h"
#include "Vulkan/Interface.h"

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

		RenderInterface(VulkanAPI::Device device, const uint32_t win_width, const uint32_t win_height);
		~RenderInterface();

		void add_static_mesh(std::unique_ptr<ComponentInterface>& comp_interface, Object& obj);

	private:

		std::vector<RenderableType> renderables;

		std::unique_ptr<VulkanAPI::Interface> vk_interface;
	};

}

