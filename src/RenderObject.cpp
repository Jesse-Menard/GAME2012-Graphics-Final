#include "RenderObject.h"

RenderObject::RenderObject(Mesh* mesh)
{
	this->mesh = mesh;
}

RenderObject::RenderObject(Vector3 pos, Vector3 scale, Vector3 rotation, Mesh* mesh, GLuint texture = NULL, GLuint normalMap = NULL, GLuint heightMap = NULL, bool shouldEmit = false)
{
	this->position = pos;// -scale / 2;
	this->scale = scale;
	this->rotationVec = rotation;
	this->texture = texture;
	this->normalMap = normalMap;
	this->heightMap = heightMap;
	this->mesh = mesh;
}

RenderObject::RenderObject(Vector3 pos, Vector3 scale, Vector3 rotation, Mesh* mesh, GLuint texture, GLuint normalMap, bool shouldEmit = false)
{
	this->position = pos;// -scale / 2;
	this->scale = scale;
	this->rotationVec = rotation;
	this->texture = texture;
	this->normalMap = normalMap;
	this->shouldEmit = shouldEmit;
	this->mesh = mesh;	

	if (shouldEmit)
		InitializeLights();
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

	if(heightMap != NULL)
	{
		glUniform1i(glGetUniformLocation(program, "u_depthMap"), 3);
		glActiveTexture(GL_TEXTURE0 + 3);
		glBindTexture(GL_TEXTURE_2D, heightMap);
	}

	DrawMesh(*mesh);
}

Vector3 RenderObject::GetCenteredPosition()
{
	return position - scale / 2; // :/
}

void RenderObject::InitializeLights()
{
	for (int i = 0; i < lightAmount; i++)
	{
		lights[i] = Light{ position + Vector3{0.0f, i * (1.5f / lightAmount), 0.0f}, {1.0f, 0.40f + i * (0.5f / lightAmount), 0.0f}, POINT_LIGHT, V3_UP };
		lights[i].intensity = 1.0f;
		lights[i].radius = 0.6f;
		lights[i].specularScale = 64.0f;
	}
}

void RenderObject::Emit()
{
	if (shouldEmit)
	{
		float posRange = 1.5f;
		float posInc = 0.1f;
		float colorRange = 0.5f;
		float colorInc = (posInc / posRange) * colorRange;
		int flickerAmount = 4;
		int tickReset = 20;

		// Updates 'flames' position & colour to try and mimic flame
		for (int i = 0; i < lightAmount; i++)
		{
			lights[i].position += lights[i].direction * posInc;
			if (lights[i].position.y > position.y + posRange * 3 / 5)
			{
				lights[i].color -=  5 * (posInc / posRange) / 2;
			}
			else
				lights[i].color.y += colorInc;

			if (ticks % (tickReset / flickerAmount) == 0)//lights[i].position.y - position.y >= (posRange/ flickerAmount * k) - posInc && lights[i].position.y - position.y <= (posRange / flickerAmount * k) + posInc)
				lights[i].direction = Normalize(Vector3{ Random(-0.6f, 0.6f), 1.0f, Random(-0.6f, 0.6f) });
		
			if (lights[i].position.y > position.y + posRange|| ticks % (tickReset* 5) == i * tickReset / lightAmount )
			{
				lights[i].position = position;
				lights[i].color = { 1.0f, 0.30f, 0.0f };
			}
		}
		ticks++;
	}
}
