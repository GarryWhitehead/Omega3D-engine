#pragma once
#include "Systems/system.h"
#include "glm.hpp"

class World;

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

class Camera
{

public:

	struct CameraDataType
	{
		float fov;
		float zNear;
		float zFar;
		float velocity;
		CameraType type;
	};


	Camera(CameraDataType camera, uint32_t width, uint32_t height);

	void SetMovementDirection(MoveDirection dir);
	void SetPitchYaw(double xpos, double ypos);
	void setPerspective(float fov, float aspect, float zNear, float zFar);
	void updateViewMatrix();

	// helper functions
	float GetZNear() const { return zNear; }
	float GetZFar() const { return zFar; }
	glm::vec3 GetCameraPosition() const { return m_cameraPos; }

private:

	glm::mat4 currentProjMatrix;
	glm::mat4 currentViewMatrix;

	MoveDirection m_currentDir;

	glm::vec3 m_cameraPos;
	glm::vec3 m_cameraFront;
	glm::vec3 m_cameraUp;
	float m_cameraYaw;
	float m_cameraPitch;

	double m_currentX;
	double m_currentY;
	bool m_isMoving;

	// Values set on init
	float zNear;
	float zFar;
	float fov;
	float velocity;
	CameraType type;
};

