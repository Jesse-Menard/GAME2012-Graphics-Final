#version 460 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTcoord;
layout (location = 3) in vec3 aTangent;

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


	/// Math from https://learnopengl.com/Advanced-Lighting/Normal-Mapping

	vec3 T = normalize(vec3(u_world * vec4(aTangent, 0.0)));
	vec3 N = normalize(vec3(u_world * vec4(aNormal, 0.0)));

	// re-orthogonalize T with respect to N
	T = normalize(T - dot(T, N) * N);

	// then retrieve perpendicular vector B with the cross product of T and N
	vec3 B = cross(N, T);
	
	TBN = mat3(T, B, N);  


   gl_Position = u_mvp * vec4(aPosition, 1.0);
}
