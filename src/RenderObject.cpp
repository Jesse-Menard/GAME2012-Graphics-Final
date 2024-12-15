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
		InitializeLights();

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

void RenderObject::InitializeLights()
{
	for (int i = 0; i < 4; i++)
	{
		lights[i] = Light{ position + Vector3{0.0f, i * 0.25f, 0.0f}, {1.0f, 0.40f + i * 0.125f, 0.0f}, POINT_LIGHT, V3_UP };
		lights[i].intensity = 1.2f;
		lights[i].radius = 0.3f;
	}
}

void RenderObject::Emit()
{
	if (shouldEmit)
	{
		float posRange = 1;
		float posInc = 0.05f;
		float colorRange = 0.5f;
		float colorInc = (posInc / posRange) * colorRange;

		for (int i = 0; i < 4; i++)
		{
			lights[i].position += lights[i].direction * posInc;
			if (lights[i].position.y > position.y + 0.80)
			{
				lights[i].color -= posInc / posRange * 0.2f * 5;
			}
			lights[i].color.y += colorInc;

			if (lights[i].position.y > position.y + posRange)
			{
				lights[i].position = position;
				lights[i].color = { 1.0f, 0.30f, 0.0f };
				lights[i].direction = Normalize(Vector3{ Random(-0.3f, 0.3f), 1.0f, Random(-0.3f, 0.3f)});
			}
		}
	}
}
