#pragma once

#include "glm.hpp"

enum class MoveDirection
{
	MOVE_FORWARD,
	MOVE_BACKWARD,
	MOVE_LEFT,
	MOVE_RIGHT,
	NO_MOVEMENT
};

class CameraSystem
{

public:
	
	struct cameraInfo
	{
		glm::mat4 projection;
		glm::mat4 viewMatrix;

	} m_cameraInfo;

	const float CAMERA_VELOCITY = 10.0f;
	const float MOUSE_SENSITIVITY = 0.1f;

	CameraSystem();
	CameraSystem(glm::vec3 cameraPos, float dt, glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f));
	~CameraSystem();

	void Update(float dt);
	void SetMovementDirection(MoveDirection dir);
	void SetPitchYaw(double xpos, double ypos);
	void SetPerspective(float fov, float aspect, float zNear, float zFar);
	void UpdateViewMatrix();

private:

	MoveDirection m_currentDir;

	glm::vec3 m_cameraPos;
	glm::vec3 m_cameraFront;
	glm::vec3 m_cameraUp;
	float m_cameraYaw;
	float m_cameraPitch;

	double m_currentX;
	double m_currentY;
	bool m_isMoving;

};

