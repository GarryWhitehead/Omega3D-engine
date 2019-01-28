#include "CameraManager.h"
#include "Engine/Omega_Global.h"

namespace OmegaEngine
{

	CameraManager::CameraManager()
	{
		// set up events
		Global::managers.eventManager->registerListener<Camera, MouseButtonEvent>(this, mouse_button_event);
		Global::managers.eventManager->registerListener<Camera, KeyboardPressEvent>(this, keyboard_press_event);

		// allocate gpu memory now for the ubo buffer as the size will remain static
		VulkanAPI::MemoryAllocator &mem_alloc = VulkanAPI::Global::Managers::mem_allocator;
		mem_alloc.allocate(VulkanAPI::MemoryUsage::VK_BUFFER_DYNAMIC, vk::BufferUsageFlagBits::eUniformBuffer, sizeof(buffer_info));
	}


	CameraManager::~CameraManager()
	{
	}

	void CameraManager::updateViewMatrix()
	{
		currentViewMatrix = OEMaths::lookAt(position, position + frontVec, upVec);
	}

	void CameraManager::mouse_button_event(MouseButtonEvent& event)
	{
		double offsetX = currentX - event.xpos;
		double offsetY = currentY - event.ypos;

		currentX = event.xpos;
		currentY = event.ypos;

		yaw -= offsetX * MOUSE_SENSITIVITY;
		pitch -= offsetY * MOUSE_SENSITIVITY;

		pitch = std::max(pitch, 89.0f);
		pitch = std::min(pitch, -89.0f);
	}

	void CameraManager::keyboard_press_event(KeyboardPressEvent& event)
	{
		Camera& camera = cameras[camera_index];

		if (event.isMoving) {

			//calculate the pitch and yaw vectors
			front_vec.x = cos(OEMaths::radians(yaw)) * cos(OEMaths::radians(pitch));
			front_vec.y = sin(OEMaths::radians(pitch));
			front_vec.z = sin(OEMaths::radians(yaw)) * cos(OEMaths::radians(pitch));
			front_vec = OEMaths::normalise_vec3(front_vec);
			
			float velocity = camera.get_Velocity();

			// check for forwards and strafe movement
			if (event.isMovingForward) {
				current_pos += front_vec * velocity * Engine::DT;
			}
			if (event.isMovingBackward) {
				current_pos -= front_vec * velocity * Engine::DT;
			}
			if (event.isMovingLeft) {
				current_pos -= OEMaths::normalise_vec3(OEMaths::cross(front_vec, camera.get_up_vec())) * velocity;
			}
			if (event.isMovingRight) {
				current_pos += OEMaths::normalise_vec3(OEMaths::cross(front_vec, camera.get_up_vec())) * velocity;
			}

			isDirty = true;
		}

	}

	void CameraManager::update_frame(double time, double dt)
	{
		if (isDirty) {

			updateViewMatrix();

			// update everything in the buffer
			buffer_info.camera_pos = current_pos;
			buffer_info.projection = currentProjMatrix;
			buffer_info.view = currentViewMatrix;
			buffer_info.zNear = cameras[camera_index].getZNear();
			buffer_info.zFar = cameras[camera_index].getZFar();

			// now update on the gpu side
			VulkanAPI::MemoryAllocator &mem_alloc = VulkanAPI::Global::Managers::mem_allocator;
			mem_alloc.mapDataToSegment(ubo_buffer, &buffer_info, sizeof(buffer_info));

			isDirty = false;
		}
	}

}