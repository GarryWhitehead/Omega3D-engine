#include "SceneManager.h"
#include "DataTypes/Camera.h"

namespace OmegaEngine
{

	SceneManager::SceneManager()
	{
	}


	SceneManager::~SceneManager()
	{
	}

	void SceneManager::setCurrentCamera(CameraDataType& cameraInfo)
	{
		currentCamera = std::make_unique<Camera>(cameraInfo);
		assert(currentCamera != nullptr);
	}
}