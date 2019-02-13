#pragma once
#include "Managers/ManagerBase.h"
#include "Managers/EventManager.h"
#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_transform.h"
#include "Vulkan/Vulkan_Global.h"

namespace OmegaEngine 
{
	// forward declerations
	class ObjectManager;

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
		MouseMoveEvent() {}
		
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

		void update_frame(double time, double dt, std::unique_ptr<ObjectManager>& obj_manager, std::unique_ptr<ComponentInterface>& component_interface) override;

		void updateViewMatrix();

		// event functions
		void keyboard_press_event(KeyboardPressEvent& event);
		void mouse_move_event(MouseMoveEvent& event);

		vk::Buffer& get_ubo_buffer()
		{
			VulkanAPI::MemoryAllocator& mem_alloc = VulkanAPI::Global::Managers::mem_allocator;
			return mem_alloc.get_memory_buffer(ubo_buffer.get_id());
		}

		uint32_t get_ubo_offset() const
		{
			return ubo_buffer.get_offset();
		}

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

		double yaw = 0.0;
		double pitch = 0.0;
		double currentX = 0.0;
		double currentY = 0.0;

		// the current camera info for sending to vulkan
		CameraBufferInfo buffer_info;

		// info for the gpu side
		VulkanAPI::MemorySegment ubo_buffer;

		// signfies whether the camera buffer needs updating both here and on the GPU side
		bool isDirty = true;
	};

}

