#pragma once
#include <memory>
#include "Vulkan/Renderpass.h"

// vulkan forward defs
namespace VulkanAPI
{
	class Interface;
}

namespace OmegaEngine
{
	// forward defs
	class RenderInterface;

	// Note: only deferred renderer is supported at the moment. More to follow....
	enum class RendererType
	{
		Deferred,
		Forward,
		Count
	};


	class RendererBase
	{

	public:

		RendererBase(RendererType _type) :
			type(_type)
		{
		}

		virtual ~RendererBase() {};

		VulkanAPI::RenderPass& get_first_pass()
		{
			return first_renderpass;
		}

		// abstract functions
		virtual void render(RenderInterface* render_interface, std::unique_ptr<VulkanAPI::Interface>& vk_interface) = 0;

	protected:

		// main renderable types renderpass - the first pass in all renderers
		VulkanAPI::RenderPass first_renderpass;

		RendererType type;
	};

}

