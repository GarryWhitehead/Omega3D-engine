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

	enum class MoveDirection
	{
		MOVE_FORWARD,
		MOVE_BACKWARD,
		MOVE_LEFT,
		MOVE_RIGHT,
		NO_MOVEMENT
	};

	// classes for event types
	struct LeftButtonEvent : public Event
	{
		double xpos;
		double ypos;
	};

	class Camera
	{

	public:

		
		Camera(CameraDataType camera, uint32_t width, uint32_t height);

		void setMovementDirection(MoveDirection dir);
		void left_button_event(LeftButtonEvent& event);
		void setPerspective(float fov, float aspect, float zNear, float zFar);
		void updateViewMatrix();

		// helper functions
		float getZNear() const { return zNear; }
		float getZFar() const { return zFar; }
		glm::vec3 getCameraPosition() const { return cameraPos; }

	private:

		glm::mat4 currentProjMatrix;
		glm::mat4 currentViewMatrix;

		MoveDirection direction;
		float yaw;
		float pitch;

		double currentX;
		double currentY;
		bool isMoving;

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



