#pragma once
#include "Math.h"
#include "Mesh.h"
#include "Light.h"

class RenderObject
{
public:
	RenderObject() {};
	RenderObject(const char* obj_path);
	RenderObject(ShapeType shape);
	RenderObject(Vector3 pos, Vector3 scale, Vector2 texScale, Vector3 facing, const char* obj_path, GLuint texture, GLuint normalMap);
	RenderObject(Vector3 pos, Vector3 scale, Vector2 texScale, Vector3 facing, ShapeType shape, GLuint texture, GLuint normalMap);
	RenderObject(Vector3 pos, Vector3 scale, Vector2 texScale, Vector3 facing, const char* obj_path, GLuint texture, GLuint normalMap, bool shouldEmit);
	~RenderObject();

	Vector3 position = V3_ZERO;
	Vector3 scale = V3_ONE;
	Vector3 facing = V3_FORWARD;
	Mesh mesh;

	GLuint texture = NULL;
	GLuint normalMap = NULL;
	GLuint specMap = NULL;

	Light light;
	bool shouldEmit = false;

	void Render(GLint program, Matrix* mvp, Matrix* world);
	Vector3 GetCenteredPosition();
	void Emit();
};
