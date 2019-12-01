#include "CameraManager.h"
#include "Core/Omega_Global.h"
#include "OEMaths/OEMaths_transform.h"
#include "VulkanAPI/BufferManager.h"

#include <algorithm>

namespace OmegaEngine
{

CameraManager::CameraManager()
{
	// set up events
	Global::eventManager()->registerListener<CameraManager, MouseMoveEvent, &CameraManager::mouseMoveEvent>(this);
	Global::eventManager()->registerListener<CameraManager, KeyboardPressEvent, &CameraManager::keyboardPressEvent>(
	    this);

	// for performance purposes, reserve a small amount of mem for the vector
	cameras.reserve(INIT_CONTAINER_SIZE);
}

CameraManager::~CameraManager()
{
}

void CameraManager::addCamera(Camera& camera)
{
	cameras.emplace_back(camera);
	cameraIndex = cameras.size() - 1;

	isDirty = true;
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
	double deltaY = currentY - event.ypos;

	currentX = event.xpos;
	currentY = event.ypos;

	yaw -= deltaX * mouseSensitivity;
	pitch -= deltaY * mouseSensitivity;

	pitch = pitch > 89.0 ? 89.0 : pitch;
	pitch = pitch < -89.0 ? -89.0 : pitch;

	updateCameraRotation();
}

void CameraManager::update()
{
	if (event.isMovingForward)
	{
		currentPosition += frontVec * velocity;    // Engine::DT;	<- Needed?
	}
	if (event.isMovingBackward)
	{
		currentPosition -= frontVec * velocity;    // Engine::DT;
	}
	if (event.isMovingLeft)
	{
		OEMaths::vec3f cameraRight = frontVec.cross(camera.cameraUp);
		cameraRight.normalise();
		currentPosition += cameraRight * velocity;
	}
	if (event.isMovingRight)
	{
		OEMaths::vec3f cameraRight = frontVec.cross(camera.cameraUp);
		cameraRight.normalise();
		currentPosition -= cameraRight * velocity;
	}

	//calculate the pitch and yaw vectors
	frontVec = OEMaths::vec3f{ std::cos(OEMaths::radians(pitch)) * std::cos(OEMaths::radians(yaw)),
		                       std::sin(OEMaths::radians(pitch)),
		                       std::cos(OEMaths::radians(pitch)) * std::sin(OEMaths::radians(yaw)) };
	frontVec.normalise();

	isupdateViewMatrix()
}

void CameraManager::updateViewMatrix()
{
	assert(!cameras.empty());
	auto& camera = cameras[cameraIndex];
	OEMaths::vec3f target = currentPosition + frontVec;
	currentViewMatrix = OEMaths::lookAt(currentPosition, target, camera.cameraUp);
}

void CameraManager::updateFrame(double time, double dt)
{
	if (isDirty)
	{
		updateViewMatrix();

		// update everything in the buffer
		cameraBuffer.mvp = currentProjMatrix * currentViewMatrix;    // * model
		cameraBuffer.cameraPosition = currentPosition;
		cameraBuffer.projection = currentProjMatrix;
		cameraBuffer.model = currentModelMatrix;    // this is just identity for now
		cameraBuffer.view = currentViewMatrix;
		cameraBuffer.zNear = cameras[cameraIndex].zNear;
		cameraBuffer.zFar = cameras[cameraIndex].zFar;

		VulkanAPI::BufferUpdateEvent event{ "Camera", (void*)&cameraBuffer, sizeof(CameraBufferInfo),
			                                VulkanAPI::MemoryUsage::VK_BUFFER_DYNAMIC };

		// let the buffer manager know that the buffers needs creating/updating via the event process
		Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);

		isDirty = false;
	}
}

}    // namespace OmegaEngine