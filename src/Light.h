#pragma once
#include <glad/glad.h>
#include "Math.h"
#include "Mesh.h"

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
	Light(Vector3 position, Vector3 color, LightType type);
	Light(Vector3 position, Vector3 color, LightType type, Vector3 direction);
	Light(Vector3 position, Vector3 color, LightType type, Vector3 direction, float FOV, float FOVbloom);

	Vector3 position = V3_ZERO;
	Vector3 direction = V3_FORWARD;
	Vector3 color = V3_ZERO;
	int type = POINT_LIGHT;

	float radius = 2.0f;
	float specularScale = 8.0f;
	float attenuationScale = 1.0f;
	float intensity = 1.0f;
	float FOV = 40;
	float FOVbloom = 10;	

	void Render(GLint program, int index);
	void DrawLight(GLint shader, Matrix *mvp, Mesh mesh, int index);
};