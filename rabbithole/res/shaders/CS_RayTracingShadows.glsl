#version 450

#include "common.h"

#define MAXLEN 1000.0
#define SHADOW 0.0000001
#define MOLLER_TRUMBORE
#define MAX_STACK_HEIGHT 100

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

float CalculateShadowForLight(vec3 positionOfOrigin, vec3 normalOfOrigin, Light light)
{
    float shadow = 1.f;

    if (light.radius <= 0.f)
    {
        return shadow;
    }
    
    vec3 lightVec = normalize(light.position.xyz - positionOfOrigin);
    float pointToLightDistance = length(light.position.xyz - positionOfOrigin);
    
    //if position is not facing light
    if (dot(normalOfOrigin, lightVec) < 0)
    {
        return shadow;
    }

    if (light.type == LightType_Point && pointToLightDistance > light.radius)
    {
        return shadow;
    }

    Ray ray;
    //hack to avoid self intersections and to get smoother shadow termination
    ray.origin = positionOfOrigin + normalOfOrigin * 0.01;
    ray.direction = lightVec;
    ray.t = pointToLightDistance;

    if (FindTriangleIntersection(ray))
        shadow = SHADOW;

    return shadow;
}

layout( local_size_x = 8, local_size_y = 8, local_size_z = 1 ) in;
void main()
{
	vec3 worldposition = imageLoad(positionGbuffer, ivec2(gl_GlobalInvocationID.xy)).rgb;
	vec3 normalGbuffer = imageLoad(normalGbuffer, ivec2(gl_GlobalInvocationID.xy)).rgb;
    
    float shadow = 1.f;

    for (uint i = 0; i < lightCount; i++)
    {
        shadow = CalculateShadowForLight(worldposition, normalGbuffer, Lights.light[i]);
	    vec4 res = vec4(shadow, 0, 0, 1);
	    imageStore(outTexture, ivec3(gl_GlobalInvocationID.xy, i), res);
    }
}

