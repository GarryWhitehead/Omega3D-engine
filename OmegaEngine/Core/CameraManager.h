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

    /**
     * Calculates the perspective projection matrix. Note: only perspective cameras supported at the moment
     * @return A 4x4 projection matrix
     */
    OEMaths::mat4f getPerspectiveMat()
    {
        return OEMaths::perspective(fov, aspect, zNear, zFar);
    }

    /**
     * Set the feild of view for this camera. Default value is 40.0degrees.
     * @param fov: the field of view in degrees
    */
    void setFov(const float fov)
    {
        this->fov = fov;
    }

    /**
     * Sets the near plane of the view fustrum
     * @param zn: the near plane
     */
    void setZNear(const float zn)
    {
        this->zNear = zn;
    }

    /**
     * Sets the far plane of the view fustrum
     * @param zf: the far plane
     */
    void setZFar(const float zf)
    {
        this->zFar = zf;
    }

    /**
     * Sets the aspect ratio of the view fustrum
     * @param asp: the aspect ratio.
     */
    void setAspect(const float asp)
    {
        this->aspect = asp;
    }

    /**
     * Sets the velocity of camera movements
     * @param vel: the velocity
     */
    void setVelocity(const float vel)
    {
        this->velocity = vel;
    }

    /**
     * Sets the camera type. Can either be first-person or third-persion view
     * Note: Only first-person camera supported at present
     * @param type: enum depicting this camera type
     */
    void setType(const CameraType type)
    {
        this->type = type;
    }

    /**
     * Sets the start position of the camera
     * @param pos: a 3d vector depicting the starting location of the camera
     */
    void setPosition(const OEMaths::vec3f& pos)
    {
        this->startPosition = pos;
    }

	void update();

private:

    /// default values
    float fov = 40.0f;
    float zNear = 0.5f;
    float zFar = 1000.0f;
    float aspect = 16.0f / 9.0f;
    float velocity = 0.1f;

    CameraType type = CameraType::FirstPerson;

    OEMaths::vec3f startPosition{ 0.0f, 0.0f, 6.0f };
    OEMaths::vec3f cameraUp{ 0.0f, 1.0f, 0.0f };
};

class CameraManager : public ComponentManager
{

public:
	
	

	CameraManager();
	~CameraManager();
    
	/**
	 * Updates the camera with the current data if has changed. Also updates on the GPU side
	 * @param time: elapsed time (not used)
	 * @param dt: time elpased since last frame (not used)
	 */
	void updateFrame(double time, double dt);

	void updateCameraRotation();
	void updateViewMatrix();

	// event functions
	void keyboardPressEvent(KeyboardPressEvent &event);
	void mouseMoveEvent(MouseMoveEvent &event);
	
	/**
	 * Adds a camera to the manager
	 * @param camera: a Camera struct detailing the camera characteristics
	 */
	void addCamera(Camera& camera);

	/**
	 * Gets the near plane setting for the current camera
	 * @return the near plane value
	 */
	float getZNear() const
	{
		return cameras[cameraIndex].zNear;
	}

	/**
	 * Gets the far plane value for the current camera
	 * @return the far plane value
	 */
	float getZFar() const
	{
		return cameras[cameraIndex].zFar;
	}

private:

	

	/// all the cameras that are currently active
	std::vector<Camera> cameras;

	/// an index to the current camera
	uint32_t cameraIndex;

	/// data calculated using the currently selected camera
	OEMaths::mat4f currentProjMatrix;
	OEMaths::mat4f currentViewMatrix;
	OEMaths::mat4f currentModelMatrix;
	OEMaths::vec3f currentPosition;
	OEMaths::vec3f frontVec{ 0.0f, 0.0f, -1.0f };

	double yaw = -45.0;
	double pitch = 0.0;
	double currentX = 0.0;
	double currentY = 0.0;

	/// controls the sensitivity of the camera to mouse movements
	float mouseSensitivity = 0.1;

	/// current camera data which will be uploaded to the gpu
	CameraBufferInfo cameraBuffer;

	/// signfies whether the camera buffer needs updating both here and on the GPU side
	bool isDirty = true;

	bool firstTime = true;
};

} // namespace OmegaEngine
