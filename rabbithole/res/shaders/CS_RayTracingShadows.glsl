#version 450

#include "common.h"

layout(rgba16, binding = 0, location = 0) readonly uniform image2D positionGbuffer;
layout(rgba16, binding = 1, location = 1) readonly uniform image2D normalGbuffer;

layout(r8, binding = 2, location = 2) writeonly uniform image2DArray outTexture;

layout (std430, binding = 3) readonly buffer VertexBuffer
{
    vec4 vertices[];
};

struct Triangle
{
    uint idx[3];
};

layout (std430, binding = 4) readonly buffer trianglesBuffer
{
    Triangle triangles[];
};

layout (std430, binding = 5) readonly buffer triangleIdxsBuffer
{
    uint triangleIndices[];
};

layout (std430, binding = 6) readonly buffer cfbvhNodesBuffer
{
    CFBVHNode cfbvhNodes[];
};

layout(binding = 7) uniform LightParams 
{
	Light[lightCount] light;
} Lights;

#define MAXLEN 1000.0
#define SHADOW 0.01
#define MOLLER_TRUMBORE
#define MAX_STACK_HEIGHT 100

bool intersectAABB(Ray ray, vec3 boxMin, vec3 boxMax) 
{
    vec3 tMin = (boxMin - ray.origin) / ray.direction;
    vec3 tMax = (boxMax - ray.origin) / ray.direction;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return tFar >= tNear;
}

bool rayTriangleIntersect( 
    const Ray ray, 
    const vec3 v0, const vec3 v1, const vec3 v2, 
    float p)
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

    if (p < t) return false;

    if (t < 0.001) return false;
    return true;
} 

bool findTriangleIntersection(Ray ray)
{
    uint stack[MAX_STACK_HEIGHT];
    uint currStackIdx = 0;
    stack[currStackIdx++] = 0;

    while(currStackIdx > 0)
    {
        uint boxIdx = stack[currStackIdx - 1];
		currStackIdx--;
        CFBVHNode currentNode = cfbvhNodes[boxIdx];
        
        if (currentNode.isLeaf == 1)
        {
            if (intersectAABB(ray, currentNode.bottom, currentNode.top))
            {
                //test triangle intersection
                uint count = currentNode.idxLeft;
                uint startIdx = currentNode.idxRight;

                for (int i = 0; i < count; i++)
                {
                    Triangle tri = triangles[triangleIndices[startIdx+i]];
                    vec4 a = vertices[tri.idx[0]];
                    vec4 b = vertices[tri.idx[1]];
                    vec4 c = vertices[tri.idx[2]];
                    
                    if (rayTriangleIntersect(ray, a.xyz, b.xyz, c.xyz, ray.t))
                    {
                        return true;
                    }
                }
            }
        }
        else
        {
            if (intersectAABB(ray, currentNode.bottom, currentNode.top))
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

float calculateShadowForLight(vec3 positionOfOrigin, vec3 normalOfOrigin, vec3 lightPosition)
{
    float shadow = 1.f;
    
    vec3 lightVec = normalize(lightPosition - positionOfOrigin);
    
    //if position is not facing light at all
    if (dot(normalOfOrigin, lightVec) < 0)
    {
        return 1.f;
    }
    
    Ray ray;
    //hack to avoid self intersections and to get smoother shadow termination
    ray.origin = positionOfOrigin + normalOfOrigin * 0.01;
    ray.direction = lightVec;
    ray.t = length(lightPosition - positionOfOrigin);

    if (findTriangleIntersection(ray))
        shadow = SHADOW;

    return shadow;
}

layout( local_size_x = 16, local_size_y = 16, local_size_z = 1 ) in;
void main()
{
	float x = gl_GlobalInvocationID.x;
	float y = gl_GlobalInvocationID.y;

	vec3 worldposition = imageLoad(positionGbuffer, ivec2(gl_GlobalInvocationID.xy)).rgb;
	vec3 normalGbuffer = imageLoad(normalGbuffer, ivec2(gl_GlobalInvocationID.xy)).rgb;

    for (int i = 0; i < lightCount; i++)
    {
        float shadow = 1.f;

        if (Lights.light[i].radius > 0)
        {
            shadow = calculateShadowForLight(worldposition, normalGbuffer, Lights.light[i].position.xyz);
        }

	    vec4 res = vec4(shadow, 0, 0, 1);
	    imageStore(outTexture, ivec3(gl_GlobalInvocationID.xy, i), res);
    }
}

