#include "CameraManager.h"
#include "Engine/Omega_Global.h"
#include "OEMaths/OEMaths_transform.h"
#include "OEMaths/OEMaths_convert.h"

#include <algorithm>

namespace OmegaEngine
{

	CameraManager::CameraManager(float sensitivity) :
		mouse_sensitivity(sensitivity)
	{
		// set up events
		Global::eventManager()->registerListener<CameraManager, MouseMoveEvent, &CameraManager::mouse_move_event>(this);
		Global::eventManager()->registerListener<CameraManager, KeyboardPressEvent, &CameraManager::keyboard_press_event>(this);

		// allocate gpu memory now for the ubo buffers
		VulkanAPI::MemoryAllocator &mem_alloc = VulkanAPI::Global::Managers::mem_allocator;
		ubo_buffer = mem_alloc.allocate(VulkanAPI::MemoryUsage::VK_BUFFER_DYNAMIC, sizeof(CameraBufferInfo));
	}


	CameraManager::~CameraManager()
	{
	}

	void CameraManager::mouse_move_event(MouseMoveEvent& event)
	{
		double offsetX = currentX - event.xpos;
		double offsetY = currentY - event.ypos;

		currentX = event.xpos;
		currentY = event.ypos;

		yaw -= offsetX * mouse_sensitivity;
		pitch -= offsetY * mouse_sensitivity;

		pitch = pitch > 89.0 ? 89.0 : pitch;
		pitch = pitch < -89.0 ? -89.0 : pitch;

		update_camera_rotation();
	}

	void CameraManager::keyboard_press_event(KeyboardPressEvent& event)
	{
		
		Camera& camera = cameras[camera_index];
		float velocity = camera.velocity;

		// check for forwards and strafe movement
		if (event.isMovingForward) {
			current_pos += front_vec * velocity; // Engine::DT;	<- Needed?
		}
		if (event.isMovingBackward) {
			current_pos -= front_vec * velocity; // Engine::DT;
		}
		if (event.isMovingLeft) {
			current_pos -= OEMaths::normalise_vec3(OEMaths::cross_vec3(front_vec, camera.camera_up)) * velocity;
		}
		if (event.isMovingRight) {
			current_pos += OEMaths::normalise_vec3(OEMaths::cross_vec3(front_vec, camera.camera_up)) * velocity;
		}

		isDirty = true;
	}

	void CameraManager::update_camera_rotation()
	{
		Camera& camera = cameras[camera_index];

		//calculate the pitch and yaw vectors
		front_vec.x = std::cos(OEMaths::radians(yaw)) * std::cos(OEMaths::radians(pitch));
		front_vec.y = std::sin(OEMaths::radians(pitch));
		front_vec.z = std::sin(OEMaths::radians(yaw)) * std::cos(OEMaths::radians(pitch));
		front_vec = OEMaths::normalise_vec3(front_vec);

		isDirty = true;
	}

	void CameraManager::updateViewMatrix()
	{
		assert(!cameras.empty());
		Camera& camera = cameras[camera_index];
		OEMaths::vec3f target = current_pos + front_vec;
		currentViewMatrix = OEMaths::lookAt(current_pos, target, camera.camera_up);
	}

	void CameraManager::update_frame(double time, double dt, std::unique_ptr<ObjectManager>& obj_manager, ComponentInterface* component_interface)
	{
		if (isDirty) {

			updateViewMatrix();

			// update everything in the buffer
			CameraBufferInfo buffer_info;

			buffer_info.mvp = currentProjMatrix * currentViewMatrix;	// * model
			buffer_info.camera_pos = current_pos;
			buffer_info.projection = currentProjMatrix;
			buffer_info.view = currentViewMatrix;
			buffer_info.zNear = cameras[camera_index].zNear;
			buffer_info.zFar = cameras[camera_index].zFar;
			
			// now update on the gpu side
			VulkanAPI::MemoryAllocator &mem_alloc = VulkanAPI::Global::Managers::mem_allocator;
			mem_alloc.mapDataToSegment(ubo_buffer, &buffer_info, sizeof(CameraBufferInfo));

			isDirty = false;
		}
	}

}