#include "Engine/engine.h"


// super simple example which loads a scene file and plays it

int main(int argc, char *argv[])
{
	// You can either init the engine with window title and window dimensions, or with an empty constructor
	// which will then use the config file
	OmegaEngine::Engine engine("Omega3D v1.0", 1280, 700);
	
	// for this example we are going to load a omega scene file
	engine.createWorld("assets/worlds/world1.json", "demo world");

	// we could load multiple world here, but for this example we will stick with one
	// now set the loop running
	engine.startLoop();

}
