/* Copyright (c) 2018-2020 Garry Whitehead
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "RenderGraph/RenderGraphBuilder.h"
#include "Rendering/Renderer.h"

#include "VulkanAPI/Shader.h"

// forward decleartions
namespace VulkanAPI
{
class ProgramManager;
class CmdBuffer;
class VkDriver;
struct VkContext;
}    // namespace VulkanAPI

namespace OmegaEngine
{
// forward declerations
class Renderer;
class OERenderableManager;
class EngineConfig;

class GBufferFillPass : public RenderStageBase
{
public:
	struct GBufferInfo
	{
		struct Textures
		{
			ResourceHandle position;
			ResourceHandle colour;
			ResourceHandle normal;
			ResourceHandle emissive;
			ResourceHandle pbr;
			ResourceHandle depth;
		} tex;

		struct Atachments
		{
			AttachmentHandle position;
			AttachmentHandle colour;
			AttachmentHandle normal;
			AttachmentHandle emissive;
			AttachmentHandle pbr;
			AttachmentHandle depth;
		} attach;
	};

	GBufferFillPass(VulkanAPI::VkDriver& vkDriver, RenderGraph& rGraph, Util::String id, OERenderableManager& rendManager, EngineConfig& config);

	// no copying
	GBufferFillPass(const GBufferFillPass&) = delete;
	GBufferFillPass& operator=(const GBufferFillPass&) = delete;
    
    bool init(VulkanAPI::ProgramManager* manager) override;
    
	/// creates a new renderpass with the required inputs/outputs and prepares the render func
	void setupPass() override;

	friend class Renderer;

	// draw callback function used in the render queue
	static void drawCallback(VulkanAPI::CmdBuffer* cmdBuffer, void* instance, RGraphContext& rgraphContext, RGraphPassContext& rpassContext);

private:
    
    VulkanAPI::VkDriver& driver;
    VulkanAPI::VkContext& vkContext;
    
	// reference to the render graph associated with this pass
	RenderGraph& rGraph;

	// the gbuffer uses data from the renderable manager, namely materials.
	// other vertex data will be from the render queue, considering visibility
	OERenderableManager& rendManager;
    EngineConfig& config;
    
	GBufferInfo gbufferInfo;
    vk::Format depthFormat;
};

}    // namespace OmegaEngine
