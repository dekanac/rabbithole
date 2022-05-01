#version 450

#include "common.h"

layout(rgba16, binding = 0, location = 0) uniform image2D positionGbuffer;
layout(rgba16, binding = 1, location = 1) uniform image2D normalGbuffer;

layout(r8, binding = 2, location = 3) uniform image2D outTexture;

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

#define EPSILON 0.0001
#define MAXLEN 1000.0
#define SHADOW 0.5
#define CULLING
#define MOLLER_TRUMBORE
#define MAX_STACK_HEIGHT 20

float ScalarTriple(vec3 a, vec3 b, vec3 c)
{
    return dot(cross(a,b), c);
}

bool intersectAABB(vec3 rayOrigin, vec3 rayDir, vec3 boxMin, vec3 boxMax) 
{
    vec3 tMin = (boxMin - rayOrigin) / rayDir;
    vec3 tMax = (boxMax - rayOrigin) / rayDir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return tFar >= tNear;
}

bool rayTriangleIntersect2( 
    vec3 p, vec3 dir, 
    vec3 a, vec3 b, vec3 c, 
    float t) 
{
    //TODO: HACK too close
    if (t < 0.14f) return false;

    vec3 q = p + dir * t;

    vec3 pq = q-p;
    vec3 pa = a-p;
    vec3 pb = b-p;
    vec3 pc = c-p;

    vec3 N = cross(pa, pb);
    float NdotRayDirection = dot(N, dir); 
    if (abs(NdotRayDirection) < EPSILON) // almost 0 
        return false; // they are parallel so they don't intersect ! 

    float u = ScalarTriple(pq, pc, pb);
    if (u < 0.0f) return false;

    float v = ScalarTriple(pq, pa, pc);
    if (v < 0.0f) return false;

    float w = ScalarTriple(pq, pb, pa);
    if (w < 0.0f) return false;

    return true;
}

bool rayTriangleIntersect3( 
    const vec3 orig, const vec3 dir, 
    const vec3 v0, const vec3 v1, const vec3 v2, 
    float p)
{ 
    vec3 v0v1 = v1 - v0; 
    vec3 v0v2 = v2 - v0; 
    vec3 pvec = cross(dir, v0v2); 
    float det = dot(v0v1, pvec); 
#ifdef CULLING 
    // if the determinant is negative the triangle is backfacing
    // if the determinant is close to 0, the ray misses the triangle
    if (det < EPSILON) return false; 
#else 
    // ray and triangle are parallel if det is close to 0
    if (fabs(det) < kEpsilon) return false; 
#endif 
    float invDet = 1 / det; 
 
    vec3 tvec = orig - v0; 
    float u = dot(tvec, pvec) * invDet; 
    if (u < 0 || u > 1) return false; 
 
    vec3 qvec = cross(tvec, v0v1); 
    float v = dot(dir, qvec) * invDet; 
    if (v < 0 || u + v > 1) return false; 
 
    float t = dot(v0v2, qvec) * invDet;

    if (p < t) return false;

    if (t < 0.1) return false;
    return true;
} 

bool rayTriangleIntersect( 
    vec3 orig, vec3 dir, 
    vec3 v0, vec3 v1, vec3 v2, 
    float t) 
{ 
    // compute plane's normal
    vec3 v0v1 = v1 - v0; 
    vec3 v0v2 = v2 - v0; 
    // no need to normalize
    vec3 N = cross(v0v1, v0v2); // N 
    float area2 = N.length(); 
 
    // Step 1: finding P
 
    // check if ray and plane are parallel ?
    float NdotRayDirection = dot(N, dir); 
    if (abs(NdotRayDirection) < EPSILON) // almost 0 
        return false; // they are parallel so they don't intersect ! 
 
    // compute d parameter using equation 2
    float d = -dot(N, v0); 
 
    // compute t (equation 3)
    t = -(dot(N, orig) + d) / NdotRayDirection; 
 
    // check if the triangle is in behind the ray
    if (t < 0) return false; // the triangle is behind 
 
    //TODO: HACK too close
    if (t < 0.1) return false;
    // compute the intersection point using equation 1
    vec3 P = orig + t * dir; 
 
    // Step 2: inside-outside test
    vec3 C; // vector perpendicular to triangle's plane 
 
    // edge 0
    vec3 edge0 = v1 - v0; 
    vec3 vp0 = P - v0; 
    C = cross(edge0, vp0); 
    if (dot(N,C) < 0) return false; // P is on the right side 
 
    // edge 1
    vec3 edge1 = v2 - v1; 
    vec3 vp1 = P - v1; 
    C = cross(edge1, vp1); 
    if (dot(N,C) < 0)  return false; // P is on the right side 
 
    // edge 2
    vec3 edge2 = v0 - v2; 
    vec3 vp2 = P - v2; 
    C = cross(edge2, vp2); 
    if (dot(N,C) < 0) return false; // P is on the right side; 
 
    return true; // this ray hits the triangle 
} 

layout( local_size_x = 16, local_size_y = 16, local_size_z = 1 ) in;
void main()
{
	float x = gl_GlobalInvocationID.x;
	float y = gl_GlobalInvocationID.y;

	vec3 worldposition = imageLoad(positionGbuffer, ivec2(gl_GlobalInvocationID.xy)).rgb;
	vec3 normalGbuffer = imageLoad(normalGbuffer, ivec2(gl_GlobalInvocationID.xy)).rgb;

    //TODO: support more lights, this is hardcoded
    vec3 lightPosition = Lights.light[3].position.xyz;

	vec3 lightVec = normalize(lightPosition - worldposition);
	
	float shadow = 1.f;

	if (dot(normalGbuffer, lightVec) < 0)
	{
        vec4 res = vec4(1, 0, 0, 1);
	    imageStore(outTexture, ivec2(gl_GlobalInvocationID.xy), res);
		return;
	}

    vec3 rayO = worldposition + normalGbuffer * 0.01;
    vec3 rayD = lightVec;

	float t = length(lightPosition - worldposition);

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
            if (intersectAABB(rayO, rayD, currentNode.bottom, currentNode.top))
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
                    
                    if (rayTriangleIntersect3(rayO, rayD, a.xyz, b.xyz, c.xyz, t))
                    {
                        shadow = 0.5;
                        break;
                    }
                }
            }
        }
        else
        {
            if (intersectAABB(rayO, rayD, currentNode.bottom, currentNode.top))
            {
                stack[currStackIdx++] = currentNode.idxLeft;
                stack[currStackIdx++] = currentNode.idxRight;
                
                if (currStackIdx > MAX_STACK_HEIGHT)
                {
                    return;
                }
             }
        }
    }

	vec4 res = vec4(shadow, 0, 0, 1);
	imageStore(outTexture, ivec2(gl_GlobalInvocationID.xy), res);
}

