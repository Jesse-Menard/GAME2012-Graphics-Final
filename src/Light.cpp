#include "Light.h"
#include <GLFW/glfw3.h>


Light::Light(Vector3 position, Vector3 color, LightType type, Vector3 direction = V3_FORWARD) :
    position(position), direction(direction), color(color), type(type)
{
}

void Light::Render(GLint program, int index)
{
    char front = *"u_light[" + index;

    //  glUniform3fv(glGetUniformLocation(program, front + "].position"), 1, &position.x);
    //  glUniform3fv(glGetUniformLocation(program, front + "].direction"), 1, &direction.x);
    //  glUniform3fv(glGetUniformLocation(program, front + "].color"), 1, &color.x);
    //  glUniform1i(glGetUniformLocation(program, front + "].type"), type);
    //  glUniform1f(glGetUniformLocation(program, front + "].specularScale"), specularScale);
    //  glUniform1f(glGetUniformLocation(program, front + "].attenuationScale"), attenuationScale);
    //  glUniform1f(glGetUniformLocation(program, front + "].intensity"), intensity);
    //  glUniform1f(glGetUniformLocation(program, front + "].FOV"), FOV);
    //  glUniform1f(glGetUniformLocation(program, front + "].FOVbloom"), FOVbloom);
}
