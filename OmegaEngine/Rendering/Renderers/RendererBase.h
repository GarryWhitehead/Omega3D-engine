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
	class RenderQueue;
	enum class SceneType;

	// Note: only deferred renderer is supported at the moment. More to follow....
	enum class RendererType
	{
		Deferred,
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

		VulkanAPI::RenderPass& get_shadow_pass()
		{
			return shadow_renderpass;
		}

		VulkanAPI::RenderPass& get_forward_pass()
		{
			return forward_pass;
		}

		// abstract functions
		virtual void render(std::unique_ptr<VulkanAPI::Interface>& vk_interface, SceneType scene_type, std::unique_ptr<RenderQueue>& render_queue) = 0;

	protected:

		// renderpasses which are common to all renderers
		// deferred renderer - first pass = gbuffer
		VulkanAPI::RenderPass first_renderpass;

		// all renderers have shadow functionality
		VulkanAPI::RenderPass shadow_renderpass;

		// forward-pass - for skybox rendering in the deferred pipeline
		VulkanAPI::RenderPass forward_pass;
		
		RendererType type;
	};

}

