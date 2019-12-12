#pragma once

namespace OmegaEngine
{

// forward decleartions
class Scene;

class Shadow
{
public:

	Shadow(Scene& scene);
	~Shadow();

	void updateBuffer();

private:
	
	Scene& scene;
};

}    // namespace OmegaEngine