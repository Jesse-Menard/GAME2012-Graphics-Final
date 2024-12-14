#version 460 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTcoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

uniform mat4 u_mvp;
uniform mat4 u_world;
uniform mat3 u_normal;

out vec3 position;
out vec3 normal;
out vec2 tcoord;
out mat3 TBN;

void main()
{
   position = (u_world * vec4(aPosition, 1.0)).xyz;
   normal = u_normal * aNormal;
   tcoord = aTcoord;
   vec3 tangent = (u_world * vec4(aTangent, 1.0)).xyz;
   vec3 bitangent = (u_world * vec4(aBitangent, 1.0)).xyz;

   vec3 T = normalize(vec3(u_world * vec4(tangent, 0.0)));
   vec3 B = normalize(vec3(u_world * vec4(bitangent, 0.0)));
   vec3 N = normalize(vec3(u_world * vec4(normal,    0.0)));
   TBN = inverse(mat3(T, B, N));
   TBN = mat3(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

   gl_Position = u_mvp * vec4(aPosition, 1.0);
}
