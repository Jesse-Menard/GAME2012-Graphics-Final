#version 460 core

in vec3 position;
in vec3 normal;
in vec2 tcoord;
in mat3 TBN;

out vec4 FragColor;

uniform vec3 u_cameraPosition;
uniform vec3 u_pointLightPosition;
uniform vec3 u_spotLightPosition;
uniform vec3 u_lightDirection;
uniform vec3 u_spotLightDirection;
uniform vec3 u_directionLightColor;
uniform vec3 u_pointLightColor;
uniform vec3 u_spotLightColor;
uniform float u_lightRadius;
uniform float u_intensity;

uniform float u_ambientFactor;
uniform float u_diffuseFactor;
uniform float u_specularPower;

uniform float u_fov;
uniform float u_fovBlend;
uniform int u_allowDirectionLight;
uniform int u_allowPointLight;
uniform int u_allowSpotLight;
uniform int u_normalToggle;

uniform sampler2D u_tex;
uniform sampler2D u_spec;
uniform sampler2D u_normalMap;

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
    vec3 lighting = phong(position, normal, u_cameraPosition, u_pointLightPosition, color, u_ambientFactor, u_diffuseFactor, u_specularPower);

    float dist = length(light - position);
    float attenuation = clamp(radius / dist, 0.0, 1.0);
    lighting *= attenuation;

    return lighting;
}

// Phong but based on direction only
vec3 direction_light(vec3 direction, vec3 normal, vec3 camera, vec3 color, float ambientFactor, float diffuseFactor, float specularPower)
{
    vec3 lighting = phong(vec3(0.0), normal, u_cameraPosition, -direction, color, u_ambientFactor, u_diffuseFactor, u_specularPower);
    return lighting;
}

// Phong but attenuated & within field of view (fov)
vec3 spot_light(vec3 position, vec3 direction, vec3 normal, vec3 camera, vec3 light, vec3 color, float ambientFactor, float diffuseFactor, float specularPower, float radius, float fov, float fovBlend)
{
    // TODO -- figure this out for yourself
    vec3 lighting = phong(position, normal, u_cameraPosition, u_spotLightPosition, color, u_ambientFactor, u_diffuseFactor, u_specularPower);
    // dot(a,b) = |a||b|cos0 <--- if thhis is bigger then FOV, = 0   /// if a and b are normalized, eq turns into dot(a,b) =cos0, acos(dot(a,b)) = 0 
    
    vec3 displacement = normalize(position - u_spotLightPosition);
    
    float angle = degrees(acos(dot(displacement, direction)));    

    // value of 1-0 with 1 being at edge of FOV, and 0 being at edge of FOV+Blend
    lighting *= clamp(1 - ((angle - fov) /fovBlend), 0, 1);
    
    return lighting;
}

void main()
{
    vec3 model = texture(u_tex, tcoord).rgb;
    vec3 normalMap = texture(u_normalMap, tcoord).rgb;
    normalMap = normalize(normalMap * 2.0 - 1.0);
    normalMap = normalize(TBN * normalMap);
    vec3 lighting;

    // The best booleans
    if(u_allowDirectionLight > 0)
        lighting += direction_light(u_lightDirection, normal, u_cameraPosition, u_directionLightColor, u_ambientFactor, u_diffuseFactor, u_specularPower);
    
    if(u_allowPointLight > 0)
        lighting += point_light(position, ((u_normalToggle > 0) ? normal : normalMap) , u_cameraPosition, u_pointLightPosition, u_pointLightColor, u_ambientFactor, u_diffuseFactor, u_specularPower, u_lightRadius);

    if(u_allowSpotLight > 0)
        lighting += spot_light(position, u_spotLightDirection, ((u_normalToggle > 0) ? normal : normalMap), u_cameraPosition, u_spotLightPosition, u_spotLightColor, u_ambientFactor, u_diffuseFactor, u_specularPower, u_lightRadius, u_fov, u_fovBlend);

    
    gl_FragColor = vec4(lighting * model * u_intensity, 1.0) ;
}

// Extra practice: Add more information to light such as ambient-diffuse-specular + intensity
// https://learnopengl.com/Lighting/Materials
// https://learnopengl.com/Lighting/Lighting-maps
