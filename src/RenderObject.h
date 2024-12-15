#pragma once
#include "Math.h"
#include "Mesh.h"
#include "Light.h"

class RenderObject
{
public:
	RenderObject() {};
	RenderObject(Mesh* mesh);
	RenderObject(Vector3 pos, Vector3 scale, Vector3 rotation, Mesh* mesh, GLuint texture, GLuint normalMap, GLuint heightMap, bool shouldEmit);
	RenderObject(Vector3 pos, Vector3 scale, Vector3 rotation, Mesh* mesh, GLuint texture, GLuint normalMap, bool shouldEmit);
	~RenderObject();

	Vector3 position = V3_ZERO;
	Vector3 scale = V3_ONE;
	Vector3 rotationVec = V3_ZERO;
	Mesh* mesh = nullptr;

	GLuint texture = NULL;
	GLuint normalMap = NULL;
	GLuint specMap = NULL;
	GLuint heightMap = NULL;

	bool shouldEmit = false;
	int lightIndex = NULL;
	const static int lightAmount = 3;
	Light lights[lightAmount];

	void Render(GLint program, Matrix* mvp, Matrix* world);
	Vector3 GetCenteredPosition();
	void InitializeLights();
	void Emit();
};
