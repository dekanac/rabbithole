#version 450

layout(location = 0) in VS_OUT 
{
    vec3 fragColors;
    vec3 fragNormal;
    vec3 fragPosition;
    vec2 fragUv;
    vec3 fragCameraPosition;
} fs_in;

layout (location = 0) out vec4 outColor;

layout (binding = 1) uniform sampler2D texSampler;

const vec3 lightPosition = vec3(0.3, 0.3, 0.3);
const vec3 lightColor = vec3(1.0, 1.0, 1.0);

void main() 
{
	vec3 color = texture(texSampler, fs_in.fragUv).rgb;

    vec3 ambient = 0.05 * color;
    vec3 lightDir = normalize(lightPosition - fs_in.fragPosition);
	vec3 normal = normalize(fs_in.fragNormal);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;
    // specular
    vec3 viewDir = normalize(fs_in.fragCameraPosition - fs_in.fragPosition);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
 
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    vec3 specular = vec3(0.02) * spec;
    outColor = vec4(ambient + diffuse + specular, 1.0);
}