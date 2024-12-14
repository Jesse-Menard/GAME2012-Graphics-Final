#pragma once
#include <glad/glad.h>
#include "Math.h"

enum LightType
{
	DIRECTION_LIGHT,
	POINT_LIGHT,
	SPOT_LIGHT
};

class Light
{
public:
	Light() {};
	Light(Vector3 position, Vector3 color, LightType type, Vector3 direction);

	Vector3 position = V3_ZERO;
	Vector3 direction = V3_FORWARD;
	Vector3 color = V3_ONE;
	LightType type = POINT_LIGHT;

	float specularScale = 1.0f;
	float attenuationScale = 1.0f;
	float intensity = 1.0f;
	float FOV = 40;
	float FOVbloom = 10;
	
	void Render(GLint program, int index);
};
