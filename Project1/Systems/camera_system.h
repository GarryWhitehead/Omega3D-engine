#pragma once
#include "Systems/system.h"
#include "glm.hpp"

class World;

enum class MoveDirection
{
	MOVE_FORWARD,
	MOVE_BACKWARD,
	MOVE_LEFT,
	MOVE_RIGHT,
	NO_MOVEMENT
};

class CameraSystem : public System
{

public:
	
	struct cameraInfo
	{
		glm::mat4 projection;
		glm::mat4 viewMatrix;

	} m_cameraInfo;

	struct LightInfo
	{
		glm::vec3 pos;
		
	};

	const float CAMERA_VELOCITY = 10.0f;
	const float MOUSE_SENSITIVITY = 0.1f;

	CameraSystem(World *world);
	~CameraSystem();

	void Init(glm::vec3 cameraPos, glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f));
	void Update() override;
	void Destroy() override;
	void SetMovementDirection(MoveDirection dir);
	void SetPitchYaw(double xpos, double ypos);
	void SetPerspective(float fov, float aspect, float zNear, float zFar);
	void UpdateViewMatrix();

	// helper functions
	float GetZNear() const { return m_zNear; }
	float GetZFar() const { return m_zFar; }
	glm::vec3 GetCameraPosition() const { return m_cameraPos; }

private:

	World *p_world;

	MoveDirection m_currentDir;

	glm::vec3 m_cameraPos;
	glm::vec3 m_cameraFront;
	glm::vec3 m_cameraUp;
	float m_cameraYaw;
	float m_cameraPitch;

	double m_currentX;
	double m_currentY;
	bool m_isMoving;

	// locally stored values
	float m_zNear;
	float m_zFar;
};

