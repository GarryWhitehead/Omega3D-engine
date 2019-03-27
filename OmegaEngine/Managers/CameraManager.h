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

		// default values
		float fov = 50.0f;
		float zNear = 1.0f;
		float zFar = 1000.0f;
		float aspect = 16.0f / 9.0f;
		float velocity = 0.1f;

		CameraType type = CameraType::FirstPerson;

		OEMaths::vec3f start_position{ 0.0f, 0.0f, -3.0f };
		OEMaths::vec3f camera_up{ 0.0f, 1.0f, 0.0f };
		
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
			// not everything in this buffer needs to be declarded in the shader but must be in this order
			OEMaths::mat4f mvp;
			OEMaths::vec3f camera_pos;
			float pad0;

			// in case we need individual matrices
			OEMaths::mat4f projection;
			OEMaths::mat4f view;
			OEMaths::mat4f model;

			// static stuff at the end
			float zNear;
			float zFar;
		};

		CameraManager(float sensitivity);
		~CameraManager();

		void update_frame(double time, double dt, std::unique_ptr<ObjectManager>& obj_manager, ComponentInterface* component_interface) override;

		void update_camera_rotation();
		void updateViewMatrix();

		// event functions
		void keyboard_press_event(KeyboardPressEvent& event);
		void mouse_move_event(MouseMoveEvent& event);

		void add_camera(Camera& camera)
		{
			cameras.push_back(camera);
			camera_index = cameras.size() - 1;

			current_pos = camera.start_position;
			currentProjMatrix = camera.getPerspectiveMat();
		}

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
		OEMaths::vec3f front_vec{ 0.0f, 0.0f, -1.0f };

		double yaw = -45.0;
		double pitch = 0.0;
		double currentX = 0.0;
		double currentY = 0.0;

		float mouse_sensitivity;

		// info for the gpu side 
		VulkanAPI::MemorySegment ubo_buffer;

		// signfies whether the camera buffer needs updating both here and on the GPU side
		bool isDirty = true;
	};

}

