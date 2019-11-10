#pragma once
#include <memory>
#include "VulkanAPI/Renderpass.h"

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

		VulkanAPI::RenderPass& getFirstPass()
		{
			return firstRenderpass;
		}

		VulkanAPI::RenderPass& getShadowPass()
		{
			return shadowRenderpass;
		}

		VulkanAPI::RenderPass& getForwardPass()
		{
			return forwardRenderpass;
		}

		// abstract functions
		virtual void render(std::unique_ptr<VulkanAPI::Interface>& vkInterface, SceneType sceneType, std::unique_ptr<RenderQueue>& renderQueue) = 0;

	protected:

		// renderpasses which are common to all renderers
		// deferred renderer - first pass = gbuffer
		VulkanAPI::RenderPass firstRenderpass;

		// all renderers have shadow functionality
		VulkanAPI::RenderPass shadowRenderpass;

		// forward-pass - for skybox rendering in the deferred pipeline
		VulkanAPI::RenderPass forwardRenderpass;
		
		RendererType type;
	};

}

