#version 450

layout (location = 0) in vec3 vertColor;
layout (location = 1) in vec3 vertNormal;
layout (location = 2) in vec3 vertPosition;

layout (location = 0) out vec4 outColor;

const vec3 lightPosition = vec3(0.3, 0.4, 0.4);
const vec3 lightColor = vec3(0.4, 0.1, 0.2);

void main() {
	
	vec3 normal = normalize(vertNormal);
	vec3 lightDir = normalize(lightPosition - vertPosition);

	float diff = max(dot(normal, lightDir), 0.3);
	vec3 diffuse = diff * lightColor;
	vec3 ambient = lightColor * 0.2;
	vec3 result = (ambient + diffuse) * vertColor;
	
	outColor = vec4(result, 1.0);
}