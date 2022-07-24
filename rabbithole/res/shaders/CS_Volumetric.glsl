#version 450

#include "common.h"

#define TEX_W 160
#define TEX_H 90
#define TEX_D 128

layout(r8, binding = 0) writeonly uniform image3D mediaDensity3DLUT;
layout(binding = 1) uniform sampler3D samplerNoise3DLUT;

layout(binding = 2) uniform LightParams 
{
	Light[lightCount] light;
} Lights;

layout(binding = 3) uniform UniformBufferObjectBuffer
{
    UniformBufferObject UBO;
};

layout(binding = 4) uniform VolumetricFogParamsBuffer
{
    VolumetricFogParams fogParams;
};

layout (std430, binding = 5) readonly buffer VertexBuffer
{
    vec4 vertices[];
};

struct Triangle
{
    uint idx[3];
};

layout (std430, binding = 6) readonly buffer TrianglesBuffer
{
    Triangle triangles[];
};

layout (std430, binding = 7) readonly buffer TriangleIdxsBuffer
{
    uint triangleIndices[];
};

layout (std430, binding = 8) readonly buffer CFBVHNodesBuffer
{
    CFBVHNode cfbvhNodes[];
};

#define MAXLEN 1000.0
#define SHADOW 0.0000001
#define MOLLER_TRUMBORE
#define MAX_STACK_HEIGHT 100

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

const float fogStartDistance = 0.07f;
const float fogDistance = 128.f;
const vec3 fogColorAmbient = vec3(0.5,0.5,0.5);
float noiseFreq = 1.0f;
vec3 simulateWind = vec3(1.f); //wind direction and speed in CPP

layout( local_size_x = 8, local_size_y = 4, local_size_z = 8 ) in;
void main()
{
    vec3 screenSpacePos = gl_GlobalInvocationID.xyz * vec3(2.0f/TEX_W, 2.0f/TEX_H, 1.0f/TEX_D) + vec3(-1,-1,0);

    float origScreenSpacePosZ = screenSpacePos.z;
    screenSpacePos.z = pow(screenSpacePos.z,1.2f);
    float densityWeight = pow(origScreenSpacePosZ + 1/128.0f, 0.2f);

    uvec3 texturePos = gl_GlobalInvocationID.xyz;

    vec3 eyeRay = (UBO.eyeXAxis.xyz * screenSpacePos.xxx + UBO.eyeYAxis.xyz * screenSpacePos.yyy + UBO.eyeZAxis.xyz);

    vec3 viewRay = eyeRay * -(screenSpacePos.z * fogDistance + fogStartDistance);
    vec3 worldPos = UBO.cameraPosition + viewRay;

    float densityVal = 1.0f; //check shadow and put shadow factor in here
    vec3 sunPosition = Lights.light[0].position.xyz;
    vec3 sunLightDirection = normalize(worldPos - sunPosition);
    vec3 sunColor = Lights.light[0].color;
    
    //TODO: deal with shadows here and update densityVal
    float shadowedSunlight = 1.0f;
    float noiseDensity = texture(samplerNoise3DLUT, screenSpacePos).r;

    Ray ray;
    ray.origin = worldPos;
    ray.direction = normalize(sunPosition - worldPos);
    ray.t = length(sunPosition - worldPos);
    
    if (FindTriangleIntersection(ray))
    {
        shadowedSunlight = 0.00001f;
        noiseDensity *= 0.01f;
    }
    else
    {
        shadowedSunlight = densityVal;
    }
    
    float verticalFalloff = 0.1f;

    float densityFinal = verticalFalloff * noiseDensity * densityWeight;

    float colorLerpFactor = 0.5f + 0.5f * dot(normalize(viewRay.xyz), sunLightDirection);

    vec3 lightIntensity = mix(fogColorAmbient, mix(sunColor, fogColorAmbient, colorLerpFactor), shadowedSunlight);

    for (int j = 1; j < 4; ++j)
    {
        vec3 incidentVector = Lights.light[j].position.xyz - worldPos;
        float distance = length(incidentVector);
        float attenutation = Lights.light[j].radius / (pow(distance, 2.0) + 1.0);
        vec3 lightColor = Lights.light[j].color;
        lightIntensity += lightColor * attenutation* attenutation;
    }

    vec4 res = vec4(lightIntensity, 1.f) * densityFinal;
    imageStore(mediaDensity3DLUT, ivec3(texturePos), res);
}