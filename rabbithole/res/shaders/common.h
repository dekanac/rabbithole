
#define lightCount 4

#define LAYOUT_OUT_VEC2(x) layout(location = x) out vec2
#define LAYOUT_IN_VEC3(x) layout(location = x) in vec3
#define LAYOUT_IN_VEC2(x) layout(location = x) in vec2
#define LAYOUT_OUT_VEC3(x) layout(location = x) out vec3

#define EPSILON 0.0001

struct Light
{
	vec4 position;
	vec3 color;
	float radius;
};

struct CFBVHNode
{
	vec3 bottom;
	vec3 top;

	uint idxLeft; //if leaf then this is count
	uint idxRight; //if leaf then this is startIdx in triangle indices
	uint isLeaf;
};

struct Ray
{
	vec3 origin;
	vec3 direction;
	float t;
};

struct UniformBufferObject
{
	mat4 view;
	mat4 proj;
	vec3 cameraPosition;
	vec3 debugOption;
	mat4 viewProjInverse;
	mat4 viewProjMatrix;
	mat4 prevViewProjMatrix;
};

float ScalarTriple(vec3 a, vec3 b, vec3 c)
{
	return dot(cross(a, b), c);
}

