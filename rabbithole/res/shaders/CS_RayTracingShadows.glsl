#version 450

#include "common.h"

#define SOFT_SHADOWS

#define MAXLEN 1000.0
#define IN_SHADOW 0.0000001
#define MOLLER_TRUMBORE
#define MAX_STACK_HEIGHT 50

layout(rgba16, binding = 0) readonly uniform image2D positionGbuffer;
layout(rgba16, binding = 1) readonly uniform image2D normalGbuffer;

layout(r8, binding = 2) writeonly uniform image2DArray outTexture;

layout (std430, binding = 3) readonly buffer VertexBuffer
{
    vec4 vertices[];
};

struct Triangle
{
    uint idx[3];
};

layout (std430, binding = 4) readonly buffer TrianglesBuffer
{
    Triangle triangles[];
};

layout (std430, binding = 5) readonly buffer TriangleIdxsBuffer
{
    uint triangleIndices[];
};

layout (std430, binding = 6) readonly buffer CFBVHNodesBuffer
{
    CFBVHNode cfbvhNodes[];
};

layout(binding = 7) uniform LightParams 
{
	Light[lightCount] light;
} Lights;

layout(rgba8, binding = 8) readonly uniform image2D noiseTexture;

layout(binding = 9) uniform UniformBufferObjectBuffer 
{
    UniformBufferObject UBO;
};

bool IntersectAABB(Ray ray, vec3 boxMin, vec3 boxMax) 
{
    vec3 tMin = (boxMin - ray.origin) / ray.direction;
    vec3 tMax = (boxMax - ray.origin) / ray.direction;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return tFar >= tNear;
}

//M�ller-Trumbore
bool RayTriangleIntersect(const Ray ray, const vec3 v0, const vec3 v1, const vec3 v2)
{ 
    vec3 v0v1 = v1 - v0; 
    vec3 v0v2 = v2 - v0; 
    vec3 pvec = cross(ray.direction, v0v2); 
    float det = dot(v0v1, pvec); 

    if (abs(det) < EPSILON) return false; 
    
    float invDet = 1 / det; 
 
    vec3 tvec = ray.origin - v0; 
    float u = dot(tvec, pvec) * invDet; 
    if (u < 0 || u > 1) return false; 
 
    vec3 qvec = cross(tvec, v0v1); 
    float v = dot(ray.direction, qvec) * invDet; 
    if (v < 0 || u + v > 1) return false; 
 
    float t = dot(v0v2, qvec) * invDet;

    if (ray.t < t) return false;

    if (t < 0.001) return false;
    return true;
} 

bool FindTriangleIntersection(Ray ray)
{
    uint stack[MAX_STACK_HEIGHT];
    uint currStackIdx = 0;
    stack[currStackIdx++] = 0;

    while(currStackIdx > 0)
    {
        uint boxIdx = stack[currStackIdx - 1];
		currStackIdx--;
        CFBVHNode currentNode = cfbvhNodes[boxIdx];
        
        vec3 bottom = vec3(currentNode.ba, currentNode.bb, currentNode.bc);
        vec3 top = vec3(currentNode.ta, currentNode.tb, currentNode.tc);

        if (isLeaf(currentNode))
        {
            if (IntersectAABB(ray, bottom, top))
            {
                //test triangle intersection
                uint count = currentNode.idxLeft - 0x80000000;
                uint startIdx = currentNode.idxRight;

                for (int i = 0; i < count; i++)
                {
                    Triangle tri = triangles[triangleIndices[startIdx+i]];
                    vec4 a = vertices[tri.idx[0]];
                    vec4 b = vertices[tri.idx[1]];
                    vec4 c = vertices[tri.idx[2]];
                    
                    if (RayTriangleIntersect(ray, a.xyz, b.xyz, c.xyz))
                    {
                        return true;
                    }
                }
            }
        }
        else
        {
            if (IntersectAABB(ray, bottom, top))
            {
                stack[currStackIdx++] = currentNode.idxLeft;
                stack[currStackIdx++] = currentNode.idxRight;
                
                if (currStackIdx > MAX_STACK_HEIGHT)
                {
                    //in case we broke stack height
                    return false;
                }
            }
        }
    }

    return false;
}

const float PI = 3.14159265359;

vec3 UpVector(vec3 forward)
{
    return abs(forward.z) < 0.999f ? vec3(0.0f, 0.0f, 1.0f) : vec3(1.0f, 0.0f, 0.0f);
}

void ComputeAxes(vec3 forward, out vec3 right, out vec3 up)
{
    up    = UpVector(forward);
    right = normalize(cross(up, forward));
    up    = normalize(cross(forward, right));
}

vec3 GetPointInDisk(vec3 center, float r, vec3 forward, vec2 noise)
{
    const float theta = noise.x * 2.0f * PI;

    const float cr = sqrt(r * noise.y);

    vec3 right;
    vec3 up;
    ComputeAxes(forward, right, up);

    const float x = cr * cos(theta);
    const float y = cr * sin(theta);

    return center + (right * x) + (up * y);
}

vec2 GetNoiseFromTexture(uvec2 aPixel, uint aSeed)
{
    uvec2 t;
    t.x = aSeed * 1664525u + 1013904223u;
    t.y = t.x * (1u << 16u) + (t.x >> 16u);
    t.x += t.y * t.x;
    uvec2 uv = ((aPixel + t) & 0x3f);
    return imageLoad(noiseTexture, ivec2(uv)).xy;
}

float CalculateShadowForLight(vec3 positionOfOrigin, vec3 normalOfOrigin, Light light, uvec2 uv)
{
    float shadow = 1.f;

    if (light.radius <= 0.f || light.intensity <= 0.f)
    {
        return shadow;
    }

    vec3 lightVec = normalize(light.position - positionOfOrigin);

#ifdef SOFT_SHADOWS
    vec2 noise = GetNoiseFromTexture(uv, uint(UBO.currentFrameInfo.x));
    noise = fract(noise + (uint(UBO.currentFrameInfo.x) ) * PI);
    lightVec = normalize(GetPointInDisk(light.position.xyz , light.size, -lightVec, noise) - positionOfOrigin);
#else
    lightVec = normalize(light.position - positionOfOrigin);
#endif
    float pointToLightDistance = distance(light.position.xyz, positionOfOrigin);
    
    //if position is not facing light
    if (dot(normalOfOrigin, lightVec) < 0)
    {
        return IN_SHADOW;
    }

    if (light.type == LightType_Point && pointToLightDistance > light.radius)
    {
        return IN_SHADOW;
    }

    Ray ray;
    ray.origin = positionOfOrigin + normalOfOrigin * 0.01;
    ray.direction = lightVec;
    ray.t = pointToLightDistance;

    if (FindTriangleIntersection(ray))
        shadow = IN_SHADOW;

    return shadow;
}

layout( local_size_x = 8, local_size_y = 8, local_size_z = 1 ) in;
void main()
{
	vec3 worldposition = imageLoad(positionGbuffer, ivec2(gl_GlobalInvocationID.xy)).rgb;
	vec3 normalGbuffer = imageLoad(normalGbuffer, ivec2(gl_GlobalInvocationID.xy)).rgb;

	vec4 res = vec4(CalculateShadowForLight(worldposition, normalGbuffer, Lights.light[gl_GlobalInvocationID.z], gl_GlobalInvocationID.xy), 0, 0, 1);
	imageStore(outTexture, ivec3(gl_GlobalInvocationID.xyz), res);
}

