#pragma once
#include "utility/EventManager.h"

#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_transform.h"

#include "Components/ComponentManager.h"

// the init number of cameras to resrve mem for.
#define INIT_CONTAINER_SIZE 10

namespace OmegaEngine
{

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
	MouseMoveEvent()
	{
	}

	MouseMoveEvent(double x, double y)
	    : xpos(x)
	    , ypos(y)
	{
	}

	double xpos = 0.0;
	double ypos = 0.0;
};

class Camera
{
public:

	enum class MoveDirection
	{
		Up,
		Down,
		Left,
		Right,
		None
	};

	/**
	 * @brief The uniform buffer used by the shaders. This is updated via the **updateFrame** function.
	*/
	struct Ubo
	{
		/// not everything in this buffer needs to be declarded in the shader but must be in this order
		OEMaths::mat4f mvp;
		OEMaths::mat4f projection;
		OEMaths::mat4f view;
		OEMaths::mat4f model;
		OEMaths::vec3f cameraPosition;
		float pad0;
		float zNear;
		float zFar;
	};

    enum class CameraType
    {
        FirstPerson,
        ThirdPerson
    };
    
	Camera() = default;

    // ================== getters ====================
  
    OEMaths::mat4f getMvpMatrix();
    
	float getZNear() const;

	float getZFar() const;

	float getFov() const;

	OEMaths::vec3f& getPos();

	OEMaths::mat4f& getProjMatrix();

	OEMaths::mat4f& getViewMatrix();

	OEMaths::mat4f& getModelMatrix();

    // ================ setters ======================
	/**
     * Calculates the perspective projection matrix. Note: only perspective cameras supported at the moment
     */
	void setPerspective();

    /**
     * Set the feild of view for this camera. Default value is 40.0degrees.
     * @param fov: the field of view in degrees
    */
	void setFov(const float camFov);

    /**
     * Sets the near plane of the view fustrum
     * @param zn: the near plane
     */
	void setZNear(const float zn);

    /**
     * Sets the far plane of the view fustrum
     * @param zf: the far plane
     */
	void setZFar(const float zf);

    /**
     * Sets the aspect ratio of the view fustrum
     * @param asp: the aspect ratio.
     */
	void setAspect(const float asp);

    /**
     * Sets the velocity of camera movements
     * @param vel: the velocity
     */
	void setVelocity(const float vel);

    /**
     * Sets the camera type. Can either be first-person or third-persion view
     * Note: Only first-person camera supported at present
     * @param type: enum depicting this camera type
     */
	void setType(const CameraType camType);

    /**
     * Sets the start position of the camera
     * @param pos: a 3d vector depicting the starting location of the camera
     */
	void setPosition(const OEMaths::vec3f& pos);

	// ============ update functions ========================
	void update();
    
    void updateViewMatrix();
    
	void updateDirection(const MoveDirection dir);

	void updatePosition(double xpos, double ypos);

private:

    /// camera attributes default values
    float fov = 40.0f;
    float zNear = 0.5f;
    float zFar = 1000.0f;
    float aspect = 16.0f / 9.0f;
    float velocity = 0.1f;
    
    // type of camera - not actually used at present
    CameraType type = CameraType::FirstPerson;
    
    // current matrices
    OEMaths::mat4f currentProj;
    OEMaths::mat4f currentView;
    OEMaths::mat4f currentModel;
    OEMaths::vec3f currentPos { 0.0f, 0.0f, 6.0f };
    OEMaths::vec3f frontVec{ 0.0f, 0.0f, -1.0f };
    
    // current status of the axis
    double yaw = -45.0;
    double pitch = 0.0;
    double currX = 0.0;
    double currY = 0.0;
    
    // "up" for this camera
    OEMaths::vec3f cameraUp{ 0.0f, 1.0f, 0.0f };

	// curren diection of movement for this camera
	MoveDirection dir = MoveDirection::None;
};



} // namespace OmegaEngine
