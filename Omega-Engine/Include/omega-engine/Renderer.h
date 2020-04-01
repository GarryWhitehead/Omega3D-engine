#pragma once

#include "utility/Compiler.h"

namespace OmegaEngine
{

class OE_PUBLIC Renderer
{

public:
	
	Renderer() = default;

	/**
     * @brief Creates all render stages needed for the rendering pipeline
     */
	void prepare();

	void beginFrame();

	/**
     * @brief Priimarily iterates over all visible renderable data within the scene and ceates the render queue.
     */
	void update();

	/**
	* @brief Draws into the cmd buffers all the data that is currently held by the scene
	*/
	void draw();

private:
};

}
