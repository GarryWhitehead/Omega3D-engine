#pragma once
#include <vector>

#include "OEMaths/OEMaths.h"

namespace OmegaEngine
{

	// forward declerations
	class Node;
	class Camera;
	class CameraDataType;

	class SceneManager
	{

	public:


		SceneManager();
		~SceneManager();

		void setCurrentCamera(CameraDataType& camera);

	private:

		std::unique_ptr<Camera> currentCamera;

		std::vector<Node> nodeBuffer;
		std::vector<Node> linearNodeBuffer;
	};

}

