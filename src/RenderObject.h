#pragma once
#include "Math.h"
#include "Mesh.h"

class RenderObject
{
public:
	RenderObject(Vector3 pos, Vector3 scale, Vector3 facing, const char* obj_path, GLuint texture, GLuint normalMap);
	RenderObject(Vector3 pos, Vector3 scale, Vector3 facing, ShapeType shape, GLuint texture, GLuint normalMap);
	~RenderObject();

	Vector3 position;
	Vector3 scale;
	Vector3 facing;
	Mesh mesh;

	GLuint texture;
	GLuint normalMap;
};

void Render();