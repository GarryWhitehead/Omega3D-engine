#include "Engine/engine.h"

#include "GLFW/glfw3.h"
#include "tiny_gltf.h"

#include <memory>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

// super simple example which loads a scene file and plays it

int main(int argc, char *argv[])
{
	// You can either init the engine with window title and window dimensions, or with an empty constructor
	// which will then use the config file
	OmegaEngine::Engine engine("Omega3D v1.0", 1280, 700);
	
	// for this example we are going to load a omega scene file
	engine.createWorld("/assets/demo-world.oes", "demo world");

	// we could load multiple world here, but for this example we will stick with one
	// now set the loop running
	engine.start_loop();

}
