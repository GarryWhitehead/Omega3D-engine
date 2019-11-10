#pragma once

#include "Core/ObjectManager.h"

#include "Models/Formats/GltfModel.h"

#include "OEMaths/OEMaths.h"

#include "utility/String.h"

#include <unordered_map>
#include <vector>

namespace VulkanAPI
{
class Device;
}

namespace OmegaEngine
{
// forward declerartions
class Scene;
class Object;
class BVH;
class AnimationManager;
class CameraManager;
class LightManager;
class RenderableManager;
class TransformManager;

class World
{
public:
	World();
	~World();

	void update(double time, double dt);

	/**
	 * prepares this world ready for recieving a scene
	 * @param name An identiying name for this world. 
	 */
	void prepare(const std::string& name);

	/**
	* @brief creates a new empty scene. This will be passed to the renderer for presentation.
	* @return A pointer to the newly created scene
	*/
	Scene* createScene();

	// pointers to each manager
	AnimationManager& getAnimManager();
	CameraManager& getCameraManager();
	LightManager& getLightManager();
	RenderableManager& getRendManager();
	TransformManager& getTransManager();

private:
	// name used to identify this world
	Util::String name;

	// and all the managers that are required to deal with each of the component types
	AnimationManager* animManager = nullptr;
	CameraManager* cameraManager = nullptr;
	LightManager* lightManager = nullptr;
	RenderableManager* rendManager = nullptr;
	TransformManager* transManager = nullptr;

	// scenes associated with this world
	std::vector<Scene> scenes;
};

}    // namespace OmegaEngine
