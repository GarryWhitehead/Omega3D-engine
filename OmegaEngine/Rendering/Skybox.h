#pragma once

#include "RenderGraph/Resources.h"

#include "Rendering/Renderer.h"

//forward declerations
namespace VulkanAPI
{
class ProgramManager;
}

namespace OmegaEngine
{
//forward declerations
class RenderGraph;

class Skybox : public RenderStageBase
{

public:
	static constexpr uint32_t indicesSize = 36;
	static constexpr uint32_t verticesSize = 24;

	struct SkyboxInstance
	{
		// vertex and index buffer memory info for the cube
		VulkanAPI::Buffer vertexBuffer;
		VulkanAPI::Buffer indexBuffer;
		uint32_t indexCount = 0;

		// the factor which the skybox will be blurred by
		float blurFactor = 0.0f;
	};

	Skybox(RenderGraph& rGraph, Util::String id);
	~Skybox();

	// not copyable
	Skybox(const Skybox&) = delete;
	Skybox& operator=(const Skybox&) = delete;

	bool prepare(VulkanAPI::ProgramManager* manager) override;


	// used to get the address of this instance
	void* getHandle() override
	{
		return this;
	}

private:

	// points to the render graph associated with this pass
	RenderGraph& rGraph;

	ResourceHandle output;
};

}    // namespace OmegaEngine
