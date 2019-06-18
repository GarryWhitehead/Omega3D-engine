#include "CameraManager.h"
#include "Engine/Omega_Global.h"
#include "OEMaths/OEMaths_transform.h"
#include "VulkanAPI/BufferManager.h"

#include <algorithm>

namespace OmegaEngine
{

	CameraManager::CameraManager(float sensitivity) :
		mouseSensitivity(sensitivity)
	{
		// set up events
		Global::eventManager()->registerListener<CameraManager, MouseMoveEvent, &CameraManager::mouseMoveEvent>(this);
		Global::eventManager()->registerListener<CameraManager, KeyboardPressEvent, &CameraManager::keyboardPressEvent>(this);
	}


	CameraManager::~CameraManager()
	{
	}

	void CameraManager::addCamera(OEMaths::vec3f& startPosition, float fov, float zNear, float zFar, float aspect, float velocity, Camera::CameraType type)
	{
		Camera camera;
		camera.startPosition = startPosition;
		camera.fov = fov;
		camera.zNear = zNear;
		camera.zFar = zFar;
		camera.aspect = aspect;
		camera.velocity = velocity;
		camera.type = type;

		currentPosition = camera.startPosition;
		currentProjMatrix = camera.getPerspectiveMat();

		cameras.emplace_back(std::move(camera));
		cameraIndex = static_cast<uint32_t>(cameras.size() - 1);
	}

	void CameraManager::mouseMoveEvent(MouseMoveEvent& event)
	{
		if (firstTime) 
		{
			currentX = event.xpos;
			currentY = event.ypos;
			firstTime = false;
		}
		
		double deltaX = event.xpos - currentX;
		double deltaY = event.ypos - currentY;

		currentX = event.xpos;
		currentY = event.ypos;

		yaw -= deltaX * mouseSensitivity;
		pitch -= deltaY * mouseSensitivity;

		pitch = pitch > 89.0 ? 89.0 : pitch;
		pitch = pitch < -89.0 ? -89.0 : pitch;

		updateCameraRotation();
	}

	void CameraManager::keyboardPressEvent(KeyboardPressEvent& event)
	{
		
		auto& camera = cameras[cameraIndex];
		float velocity = camera.velocity;

		// check for forwards and strafe movement
		if (event.isMovingForward) 
		{
			currentPosition += frontVec * velocity; // Engine::DT;	<- Needed?
		}
		if (event.isMovingBackward) 
		{
			currentPosition -= frontVec * velocity; // Engine::DT;
		}
		if (event.isMovingLeft) 
		{
			OEMaths::vec3f cameraRight = frontVec.cross(camera.cameraUp);
			cameraRight.normalise();
			currentPosition +=  cameraRight * velocity;
		}
		if (event.isMovingRight) 
		{
			OEMaths::vec3f cameraRight = frontVec.cross(camera.cameraUp);
			cameraRight.normalise();
			currentPosition -= cameraRight * velocity;
		}

		isDirty = true;
	}

	void CameraManager::updateCameraRotation()
	{
		auto& camera = cameras[cameraIndex];

		//calculate the pitch and yaw vectors
		frontVec = OEMaths::vec3f{ std::cos(OEMaths::radians(yaw)) * std::cos(OEMaths::radians(pitch)),
									std::sin(OEMaths::radians(pitch)),
									std::sin(OEMaths::radians(yaw)) * std::cos(OEMaths::radians(pitch)) };
		frontVec.normalise();
	
		isDirty = true;
	}

	void CameraManager::updateViewMatrix()
	{
		assert(!cameras.empty());
		auto& camera = cameras[cameraIndex];
		OEMaths::vec3f target = currentPosition + frontVec;
		currentViewMatrix = OEMaths::lookAt(currentPosition, target, camera.cameraUp);
	}

	void CameraManager::updateFrame(double time, double dt, std::unique_ptr<ObjectManager>& objectManager, ComponentInterface* componentInterface)
	{
		if (isDirty) 
		{
			updateViewMatrix();

			// update everything in the buffer
			cameraBuffer.mvp = currentProjMatrix * currentViewMatrix;	// * model
			cameraBuffer.cameraPosition = currentPosition;
			cameraBuffer.projection = currentProjMatrix;
			cameraBuffer.model = currentModelMatrix;		// this is just identity for now
			cameraBuffer.view = currentViewMatrix;
			cameraBuffer.zNear = cameras[cameraIndex].zNear;
			cameraBuffer.zFar = cameras[cameraIndex].zFar;
			
			VulkanAPI::BufferUpdateEvent event{ "Camera", (void*)&cameraBuffer, sizeof(CameraBufferInfo), VulkanAPI::MemoryUsage::VK_BUFFER_DYNAMIC };

			// let the buffer manager know that the buffers needs creating/updating via the event process
			Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);

			isDirty = false;
		}
	}

}