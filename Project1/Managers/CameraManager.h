#pragma once
#include "Managers/ManagerBase.h"
#include "Managers/EventManager.h"
#include "OEMaths/OEMaths.h"
#include "Vulkan/Vulkan_Global.h"
#include "Vulkan/MemoryAllocator.h"

namespace OmegaEngine 
{

	struct Camera
	{
		enum class CameraType
		{
			FirstPerson,
			ThirdPerson
		};

		OEMaths::mat4f getPerspectiveMat()
		{
			return OEMaths::perspective(fov, aspect, zNear, zFar);
		}

		float fov;
		float zNear;
		float zFar;
		float aspect;
		float velocity;

		CameraType type;

		OEMaths::vec3f start_position;
		OEMaths::vec3f camera_up;
		
	};

	// classes for event types
	struct MouseButtonEvent : public Event
	{
	
	};

	struct KeyboardPressEvent : public Event
	{
		bool isMovingLeft = false;
		bool isMovingRight = false;
		bool isMovingForward = false;
		bool isMovingBackward = false;
		bool isMoving = false;
	};

	struct MouseMoveEvent : public Event
	{
		MouseMoveEvent(double x, double y) :
			xpos(x), ypos(y)
		{}

		double xpos = 0.0;
		double ypos = 0.0;
	};

	class CameraManager : public ManagerBase
	{

	public:

		// data that will be used by shaders
		struct CameraBufferInfo
		{
			OEMaths::mat4f projection;
			OEMaths::mat4f view;
			OEMaths::mat4f model;

			OEMaths::vec3f camera_pos;

			float zNear;
			float zFar;
		};

		CameraManager();
		~CameraManager();

		void update_frame(double time, double dt) override;

		void updateViewMatrix();

		// event functions
		void keyboard_press_event(KeyboardPressEvent& event);
		void mouse_button_event(MouseButtonEvent& event);

	private:

		// all the cameras that had been added to the manager
		std::vector<Camera> cameras;

		// current camera
		uint32_t camera_index;

		// data calculated using the currently selected camera
		OEMaths::mat4f currentProjMatrix;
		OEMaths::mat4f currentViewMatrix;
		OEMaths::vec3f current_pos;
		OEMaths::vec3f front_vec;

		float yaw;
		float pitch;
		double currentX;
		double currentY;

		// the current camera info for sending to vulkan
		CameraBufferInfo buffer_info;

		// info for the gpu side
		VulkanAPI::MemorySegment ubo_buffer;

		// signfys whether the camera buffer needs updating both here and on the GPU side
		bool isDirty = true;
	};

}

