#version 450

#define lightCount 4

layout (location = 0) in vec2 inUV;

layout (binding = 0) uniform sampler2D samplerAlbedo;
layout (binding = 1) uniform sampler2D samplerNormal;
layout (binding = 2) uniform sampler2D samplerposition;

layout (location = 0) out vec4 outColor;

layout(binding = 3) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
	vec3 cameraPosition;
    vec3 debugOption;
} UBO;

struct Light
{
	vec4 position;
	vec4 colorAndRadius; //radius is 4th element
};

layout(binding = 4) uniform LightParams {
	Light[lightCount] light;
} Lights;



void main() 
{
    // Get G-Buffer values
    vec3 normal = texture(samplerNormal, inUV).rgb;
    vec3 position = texture(samplerposition, inUV).rgb;
    vec4 albedo = texture(samplerAlbedo, inUV);
	
	// Debug display
	if (int(UBO.debugOption.x) > 0) {
		switch (int(UBO.debugOption.x)) {
			case 1: 
				outColor.rgb = position;
				break;
			case 2: 
				outColor.rgb = normal;
				break;
			case 3: 
				outColor.rgb = albedo.rgb;
				break;
			case 4: 
				outColor.rgb = albedo.aaa;
				break;
		}		
		outColor.a = 1.0;
		return;
	}

	// Render-target composition
	
	#define ambient 0.03
	
	// Ambient part
	vec3 fragcolor  = albedo.rgb * ambient;
	
	for(int i = 0; i < lightCount; ++i)
	{
		// Vector to light
		vec3 L = Lights.light[i].position.xyz - position;
		// Distance from light to fragment position
		float dist = length(L);

		// Viewer to fragment
		vec3 V = UBO.cameraPosition.xyz - position;
		V = normalize(V);
		
		//if(dist < ubo.lights[i].radius)
		{
			// Light to fragment
			L = normalize(L);

			// Attenuation
			float atten = Lights.light[i].colorAndRadius.w / (pow(dist, 2.0) + 1.0);

			// Diffuse part
			vec3 N = normalize(normal);
			float NdotL = max(0.0, dot(N, L));
			vec3 diff = Lights.light[i].colorAndRadius.xyz * albedo.rgb * NdotL * atten;

			// Specular part
			// Specular map values are stored in alpha of albedo mrt
			//vec3 R = reflect(-L, N);
			//float NdotR = max(0.0, dot(R, V));
			//vec3 spec = testLight[i].color * albedo.a * pow(NdotR, 16.0) * atten;

			fragcolor += diff;
		}	
	}    	
   
  outColor = vec4(fragcolor, 1.0);	

}