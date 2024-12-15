#version 460 core

in vec3 position;
in vec3 normal;
in vec2 tcoord;
in mat3 TBN;
in vec3 tangentFragmentPos;

out vec4 FragColor;

uniform vec3 u_cameraPosition;

uniform float u_ambientFactor;
uniform float u_diffuseFactor;
uniform float u_heightScale;
uniform int u_normalToggle;

uniform sampler2D u_tex;
uniform sampler2D u_spec;
uniform sampler2D u_normalMap;
uniform sampler2D u_depthMap;

struct Light {
    vec3 position;
    vec3 direction;
    vec3 color;
    int type; // 0 = DIR, 1 = POINT, 2 = SPOT
    
    float radius;
    float specularScale;
	float attenuationScale;
	float intensity;
	float FOV;
	float FOVbloom;
};
uniform Light u_lights[40];

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
    float height = texture(u_depthMap, texCoords).r;
    vec2 p = viewDir.xy / viewDir.z * (height * u_heightScale);
    return texCoords - p;
}

vec3 phong(vec3 position, vec3 normal, vec3 camera, vec3 light, vec3 color, float ambientFactor, float diffuseFactor, float specularPower)
{
    vec3 N = normalize(normal);
    vec3 L = normalize(light - position);
    vec3 V = normalize(camera - position);
    vec3 R = normalize(reflect(-L, N));
    float dotNL = max(dot(N, L), 0.0);
    float dotVR = max(dot(V, R), 0.0);

    vec3 lighting = vec3(0.0);
    vec3 ambient = color * ambientFactor;
    vec3 diffuse = color * dotNL * diffuseFactor * texture(u_tex, tcoord).rgb;
    vec3 specular = color * pow(dotVR, specularPower) * texture(u_spec, tcoord).rgb;

    lighting += ambient;
    lighting += diffuse;
    lighting += specular;
    return lighting;
}

// Phong but attenuated
vec3 point_light(vec3 position, vec3 normal, vec3 camera, vec3 light, vec3 color, float ambientFactor, float diffuseFactor, float specularPower, float radius)
{
    vec3 lighting = phong(position, normal, camera, light, color, ambientFactor, diffuseFactor, specularPower);

    float dist = length(light - position);
    float attenuation = clamp(radius / dist, 0.0, 1.0);
    lighting *= attenuation;

    return lighting;
}

// Phong but based on direction only
vec3 direction_light(vec3 direction, vec3 normal, vec3 camera, vec3 color, float ambientFactor, float diffuseFactor, float specularPower)
{
    vec3 lighting = phong(vec3(0.0), normal, camera, -direction, color, ambientFactor, diffuseFactor, specularPower);
    return lighting;
}

// Phong but attenuated & within field of view (fov)
vec3 spot_light(vec3 position, vec3 direction, vec3 normal, vec3 camera, vec3 light, vec3 color, float ambientFactor, float diffuseFactor, float specularPower, float radius, float fov, float fovBloom)
{
    vec3 lighting = phong(position, normal, camera, light, color, ambientFactor, diffuseFactor, specularPower);
    
    // dot(a,b) = |a||b|cos0 <--- if thhis is bigger then FOV, = 0   /// if a and b are normalized, eq turns into dot(a,b) =cos0, acos(dot(a,b)) = 0 
    vec3 displacement = normalize(position - light);
    
    float angle = degrees(acos(dot(displacement, normalize(direction))));    

    // value of 1-0 with 1 being at edge of FOV, and 0 being at edge of FOV + Bloom
    lighting *= clamp(1 - ((angle - fov) /fovBloom), 0, 1);
    
    return lighting;
}

void main()
{
    vec3 viewDirection = normalize((TBN * u_cameraPosition) - tangentFragmentPos);
    vec2 newTcoord = ParallaxMapping(tcoord, viewDirection);

    //if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
    //    discard;

    vec3 model = texture(u_tex, newTcoord).rgb;
    vec3 normalMap = texture(u_normalMap, newTcoord).rgb;
    normalMap = normalize(normalMap * 2.0 - 1.0);
    normalMap = normalize(TBN * normalMap);


    vec3 lighting;

    for( int i = 0; i < 40; i++)
    {
        if(u_lights[i].type == 0)
            lighting += direction_light(u_lights[i].direction, ((u_normalToggle > 0) ? normal : normalMap), u_cameraPosition, u_lights[i].color, u_ambientFactor, u_diffuseFactor, u_lights[i].specularScale);
        if(u_lights[i].type == 1)
            lighting += point_light(position, ((u_normalToggle > 0) ? normal : normalMap) , u_cameraPosition, u_lights[i].position, u_lights[i].color, u_ambientFactor, u_diffuseFactor, u_lights[i].specularScale, u_lights[i].radius);
        if(u_lights[i].type == 2)
            lighting += spot_light(position, u_lights[i].direction, ((u_normalToggle > 0) ? normal : normalMap), u_cameraPosition, u_lights[i].position, u_lights[i].color, u_ambientFactor, u_diffuseFactor, u_lights[i].specularScale, u_lights[i].radius, u_lights[i].FOV, u_lights[i].FOVbloom);

        lighting *= u_lights[i].intensity;
    }
    
    gl_FragColor = vec4(lighting * model, 1.0) ;
}

// Extra practice: Add more information to light such as ambient-diffuse-specular + intensity
// https://learnopengl.com/Lighting/Materials
// https://learnopengl.com/Lighting/Lighting-maps
