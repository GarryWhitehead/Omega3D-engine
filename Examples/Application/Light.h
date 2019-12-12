#pragma once

class Light
{
public:

	enum class MovementType
	{
		Static,
		RotateX,
		RotateY,
		RotateZ
	};

	struct AnimateInfo
	{
		AnimateInfo() = default;

		AnimateInfo(const MovementType type, const float vel)
		    : animationType(type)
		    , velocity(vel)
		{
		}

		// default to static
		MovementType animationType = MovementType::Static;
		float velocity = 5.0f;
	};

	void updateLightPositions(double dt);

private:

	// use object handles - stored here? - or light handles to update light positions inb the manager per frame
};