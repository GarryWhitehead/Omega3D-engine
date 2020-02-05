#pragma once

#include "utility/Compiler.h"

#include "OEMaths/OEMaths.h"

namespace OmegaEngine
{

class OE_PUBLIC Camera
{
public:

	enum class CameraType
	{
		FirstPerson,
		ThirdPerson
	};

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

	void prepare();

protected:

	Camera() = default;
};

}    // namespace OmegaEngine
