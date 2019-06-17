#pragma once
#include "Managers/ManagerBase.h"
#include "Managers/EventManager.h"
#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_transform.h"

namespace OmegaEngine 
{
	// forward declerations
	class ObjectManager;

	class Camera
	{
	public:

		enum class CameraType
		{
			FirstPerson,
			ThirdPerson
		};

		Camera(float _fov, float _zNear, float _zFar, float _aspect, float _velocity, CameraType _type, OEMaths::vec3f& _startPosition) :
			startPosition(_startPosition),
			fov(_fov),
			zNear(_zNear),
			zFar(_zFar),
			velocity(_velocity),
			type(_type),
			aspect(_aspect)
		{}

		Camera(float _fov, float _zNear, float _zFar, OEMaths::vec3f& _startPosition) :
			startPosition(_startPosition),
			fov(_fov),
			zNear(_zNear),
			zFar(_zFar)
		{}

		// default constructor - use explicit values
		Camera() {}

		OEMaths::mat4f getPerspectiveMat()
		{
			return OEMaths::perspective(fov, aspect, zNear, zFar);
		}

		// the getters
		float getFov() const
		{
			return fov;
		}

		float getZNear() const
		{
			return zNear;
		}

		float getZFar() const
		{
			return zFar;
		}

		float getAspect() const
		{
			return aspect;
		}

		float getVelocity() const
		{
			return velocity;
		}

		CameraType getCameraType() const
		{
			return type;
		}

		OEMaths::vec3f& getPosition()
		{
			return startPosition;
		}

		OEMaths::vec3f& getCameraUp()
		{
			return cameraUp;
		}

	private:

		// default values
		float fov = 40.0f;
		float zNear = 0.5f;
		float zFar = 1000.0f;
		float aspect = 16.0f / 9.0f;
		float velocity = 0.1f;

		CameraType type = CameraType::FirstPerson;

		OEMaths::vec3f startPosition{ 0.0f, 0.0f, 6.0f };
		OEMaths::vec3f cameraUp{ 0.0f, 1.0f, 0.0f };
		
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
			
			// in case we need individual matrices
			OEMaths::mat4f projection;
			OEMaths::mat4f view;
			OEMaths::mat4f model;

			OEMaths::vec3f cameraPosition;
			float pad0;

			// static stuff at the end
			float zNear;
			float zFar;
		};

		CameraManager(float sensitivity);
		~CameraManager();

		void updateFrame(double time, double dt, std::unique_ptr<ObjectManager>& objectManager, ComponentInterface* componentInterface) override;

		void updateCameraRotation();
		void updateViewMatrix();

		// event functions
		void keyboardPressEvent(KeyboardPressEvent& event);
		void mouseMoveEvent(MouseMoveEvent& event);

		// user functions to create a new camera instance for using when calling **addCamera** 
		static std::unique_ptr<Camera> createCamera(float _fov, float _zNear, float _zFar, float _aspect, float _velocity, Camera::CameraType _type, OEMaths::vec3f& _startPosition);
		static std::unique_ptr<Camera> createCamera(float _fov, float _zNear, float _zFar, OEMaths::vec3f& _startPosition);
		static std::unique_ptr<Camera> createCamera();

		void addCamera(std::unique_ptr<Camera> camera);

		float getZNear() const
		{
			return cameras[cameraIndex]->getZNear();
		}

		float getZFar() const
		{
			return cameras[cameraIndex]->getZFar();
		}

	private:

		// all the cameras that had been added to the manager
		std::vector<std::unique_ptr<Camera> > cameras;

		// current camera
		uint32_t cameraIndex;

		// data calculated using the currently selected camera
		OEMaths::mat4f currentProjMatrix;
		OEMaths::mat4f currentViewMatrix;
		OEMaths::mat4f currentModelMatrix;
		OEMaths::vec3f currentPosition;
		OEMaths::vec3f frontVec{ 0.0f, 0.0f, -1.0f };

		double yaw = -45.0;
		double pitch = 0.0;
		double currentX = 0.0;
		double currentY = 0.0;

		float mouseSensitivity;

		// current camera data which will be uploaded to the gpu
		CameraBufferInfo cameraBuffer;

		// signfies whether the camera buffer needs updating both here and on the GPU side
		bool isDirty = true;

		bool firstTime = true;
	};

}

