#pragma once
#include "RenderableBase.h"

namespace OmegaEngine
{

	class RenderableSkybox : public RenderableBase
	{

	public:

		struct SkyboxInstance
		{
			// vertex and index buffer memory info for the cube
			VulkanAPI::Buffer vertex_buffer;
			VulkanAPI::Buffer index_buffer;
			uint32_t index_count = 0;

			// the factor which the skybox will be blurred by
			float blur_factor = 0.0f;
		};

		RenderableSkybox();
		~RenderableSkybox();
		
		// used to get the address of this instance
		void* get_handle() override
		{
			return this;
		}

		void render(VulkanAPI::SecondaryCommandBuffer& cmd_buffer, 
					void* instance_data,
					RenderInterface* render_interface) override;
					
	private:

	};

}

