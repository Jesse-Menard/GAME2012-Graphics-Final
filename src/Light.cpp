#include "Light.h"
#include <GLFW/glfw3.h>
#include <string>
#include "Mesh.h"

Light::Light(Vector3 position, Vector3 color, LightType type)
{
    this->position = position;
    this->color = color;
    this->type = type;
    Light();
}

Light::Light(Vector3 position, Vector3 color, LightType type, Vector3 direction)
{
    this->position = position;
    this->direction = direction;
    this->color = color;
    this->type = type;
    Light();
}

Light::Light(Vector3 position, Vector3 color, LightType type, Vector3 direction, float FOV, float FOVbloom)
{
    this->position = position;
    this->direction = direction;
    this->color = color;
    this->type = type;
    this->FOV = FOV;
    this->FOVbloom = FOVbloom;
    Light();
}

void Light::Render(GLint program, int index)
{
    glUseProgram(program);
    // Writes to the equivilant array of Lights in shader

    std::string location = "u_lights[" + std::to_string(index) + "].position";
    glUniform3fv(glGetUniformLocation(program, location.c_str()), 1, &position.x);
    location = "u_lights[" + std::to_string(index) + "].direction";
    glUniform3fv(glGetUniformLocation(program, location.c_str()), 1, &direction.x);
    location = "u_lights[" + std::to_string(index) + "].color";
    glUniform3fv(glGetUniformLocation(program, location.c_str()), 1, &color.x);
    location = "u_lights[" + std::to_string(index) + "].radius";
    glUniform1fv(glGetUniformLocation(program, location.c_str()), 1, &radius);
    location = "u_lights[" + std::to_string(index) + "].specularScale";
    glUniform1fv(glGetUniformLocation(program, location.c_str()), 1, &specularScale);
    location = "u_lights[" + std::to_string(index) + "].attenuationScale";
    glUniform1fv(glGetUniformLocation(program, location.c_str()), 1, &attenuationScale);
    location = "u_lights[" + std::to_string(index) + "].intensity";
    glUniform1fv(glGetUniformLocation(program, location.c_str()), 1, &intensity);
    location = "u_lights[" + std::to_string(index) + "].FOV";
    glUniform1fv(glGetUniformLocation(program, location.c_str()), 1, &FOV);
    location = "u_lights[" + std::to_string(index) + "].FOVbloom";
    glUniform1fv(glGetUniformLocation(program, location.c_str()), 1, &FOVbloom);
    location = "u_lights[" + std::to_string(index) + "].type";
    glUniform1i(glGetUniformLocation(program, location.c_str()), type);
}

void Light::DrawLight(GLint program, Matrix* mvp, Mesh mesh)
{
    glUseProgram(program);

    //  Couldn't figure it out
    //  Matrix world = Scale(V3_ONE * radius) * Translate(position);
    //  *mvp = *mvp * world;

    glUniformMatrix4fv(glGetUniformLocation(program, "u_mvp"), 1, GL_FALSE, ToFloat16(*mvp).v);
    glUniform3fv(glGetUniformLocation(program, "u_color"), 1, &color.x);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    DrawMesh(mesh);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
