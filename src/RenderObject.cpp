#include "RenderObject.h"

RenderObject::RenderObject(const char* obj_path)
{
	CreateMesh(&mesh, obj_path);
}

RenderObject::RenderObject(ShapeType shape)
{
	CreateMesh(&mesh, shape);
}

RenderObject::RenderObject(Vector3 pos, Vector3 scale, Vector2 texScale, Vector3 facing, const char* obj_path, GLuint texture = NULL, GLuint normalMap = NULL)
{
	this->position = pos;// -scale / 2;
	this->scale = scale;
	this->facing = facing;
	this->texture = texture;
	this->normalMap = normalMap;
	CreateMesh(&mesh, obj_path, texScale);
}

RenderObject::RenderObject(Vector3 pos, Vector3 scale, Vector2 texScale, Vector3 facing, ShapeType shape, GLuint texture = NULL, GLuint normalMap = NULL)
{
	this->position = pos;// -scale / 2;
	this->scale = scale;
	this->facing = facing;
	this->texture = texture;
	this->normalMap = normalMap;
	CreateMesh(&mesh, shape, texScale);
}

RenderObject::RenderObject(Vector3 pos, Vector3 scale, Vector2 texScale, Vector3 facing, const char* obj_path, GLuint texture, GLuint normalMap, bool shouldEmit)
{
	this->position = pos;// -scale / 2;
	this->scale = scale;
	this->facing = facing;
	this->texture = texture;
	this->normalMap = normalMap;
	this->shouldEmit = shouldEmit;

	if (shouldEmit)
		light = Light{ position, {0.8f, 0.8f, 0.0f}, POINT_LIGHT };
	CreateMesh(&mesh, obj_path, texScale);
}

RenderObject::~RenderObject()
{
}

void RenderObject::Render(GLint program, Matrix *mvp, Matrix *world)
{
	glUseProgram(program);
	
	glUniformMatrix4fv(glGetUniformLocation(program, "u_mvp"), 1, GL_FALSE, ToFloat16(*mvp).v);
	glUniformMatrix4fv(glGetUniformLocation(program, "u_world"), 1, GL_FALSE, ToFloat16(*world).v);

	if (texture != NULL)
	{
		glUniform1i(glGetUniformLocation(program, "u_tex"), 0);
		glActiveTexture(GL_TEXTURE0); 
		glBindTexture(GL_TEXTURE_2D, texture);
	}

	if (normalMap != NULL)
	{
		glUniform1i(glGetUniformLocation(program, "u_normalMap"), 1);
		glActiveTexture(GL_TEXTURE0 + 1); // this caught me up for so long.. lol
		glBindTexture(GL_TEXTURE_2D, normalMap);
	}

	if(specMap != NULL)
	{
		glUniform1i(glGetUniformLocation(program, "u_Spec"), 2);
		glActiveTexture(GL_TEXTURE0 + 2);
		glBindTexture(GL_TEXTURE_2D, specMap);
	}

	DrawMesh(mesh);
}

Vector3 RenderObject::GetCenteredPosition()
{
	return position - scale / 2; // :/
}

void RenderObject::Emit()
{
	if (shouldEmit)
	{
		light.position.y += 0.1f;
		if (light.position.y > position.y + 10)
			light.position = position;
	}
}
