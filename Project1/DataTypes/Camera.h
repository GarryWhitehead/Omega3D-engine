#pragma once
#include "Managers/EventManager.h"

#include "glm.hpp"

namespace OmegaEngine
{

	struct CameraDataType
	{
		float fov;
		float zNear;
		float zFar;
		float velocity;
		CameraType type;
		glm::vec3 position;
		glm::vec3 cameraUp;
	};

	enum class CameraType
	{
		FirstPerson,
		ThirdPerson
	};

	// classes for event types
	struct MouseButtonEvent : public Event
	{
		double xpos;
		double ypos;
	};

	struct KeyboardPressEvent : public Event
	{
		bool isMovingLeft = false;
		bool isMovingRight = false;
		bool isMovingForward = false;
		bool isMovingBackward = false;
		bool isMoving = false;
	};


	class Camera
	{

	public:

		Camera(CameraDataType camera, uint32_t width, uint32_t height);

		void setNearFarPlane(float near, float far);
		void setPerspective(float fov, float aspect, float zNear, float zFar);
		void updateViewMatrix();

		// event functions
		void keyboard_press_event(KeyboardPressEvent& event);
		void mouse_button_event(MouseButtonEvent& event);

		// helper functions
		float getZNear() const { return zNear; }
		float getZFar() const { return zFar; }
		glm::vec3 getCameraPosition() const { return position; }

	private:

		glm::mat4 currentProjMatrix;
		glm::mat4 currentViewMatrix;

		float yaw;
		float pitch;
		double currentX;
		double currentY;

		// Values set on init
		float zNear;
		float zFar;
		float fov;
		float velocity;
		CameraType type;

		glm::vec3 position;
		glm::vec3 frontVec;
		glm::vec3 upVec;
	};
}



