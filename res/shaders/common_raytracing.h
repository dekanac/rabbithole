#define MAXLEN 1000.0
#define IN_SHADOW 0.0000001
#define MOLLER_TRUMBORE
#define MAX_STACK_HEIGHT 50

struct Triangle
{
	uint idx[3];
};

struct Ray
{
    float3 origin;
    float3 direction;
    float t;
};

struct RayHit
{
    bool hit;
    float t;
};

[[vk::binding(0)]] StructuredBuffer<float4> VertexBuffer;
[[vk::binding(1)]] StructuredBuffer<Triangle> TrianglesBuffer;
[[vk::binding(2)]] StructuredBuffer<uint> TriangleIdxsBuffer;
[[vk::binding(3)]] StructuredBuffer<CFBVHNode> CFBVHNodesBuffer;

bool IntersectAABB(Ray ray, float3 boxMin, float3 boxMax)
{
    float3 tMin = (boxMin - ray.origin) / ray.direction;
    float3 tMax = (boxMax - ray.origin) / ray.direction;
    float3 t1 = min(tMin, tMax);
    float3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return tFar >= tNear;
}

// Moller-Trumbore
bool RayTriangleIntersect(const Ray ray, const float3 v0, const float3 v1, const float3 v2)
{
    float3 v0v1 = v1 - v0;
    float3 v0v2 = v2 - v0;
    float3 pvec = cross(ray.direction, v0v2);
    float det = dot(v0v1, pvec);

    if (abs(det) < EPSILON) return false;

    float invDet = 1 / det;

    float3 tvec = ray.origin - v0;
    float u = dot(tvec, pvec) * invDet;
    if (u < 0 || u > 1) return false;

    float3 qvec = cross(tvec, v0v1);
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

    while (currStackIdx > 0)
    {
        uint boxIdx = stack[currStackIdx - 1];
        currStackIdx--;
        CFBVHNode currentNode = CFBVHNodesBuffer[boxIdx];

        float3 bottom = float3(currentNode.ba, currentNode.bb, currentNode.bc);
        float3 top = float3(currentNode.ta, currentNode.tb, currentNode.tc);

        if (isLeaf(currentNode))
        {
            if (IntersectAABB(ray, bottom, top))
            {
                // Test triangle intersection
                uint count = currentNode.idxLeft - 0x80000000;
                uint startIdx = currentNode.idxRight;

                for (int i = 0; i < count; i++)
                {
                    Triangle tri = TrianglesBuffer[TriangleIdxsBuffer[startIdx + i]];
                    float4 a = VertexBuffer[tri.idx[0]];
                    float4 b = VertexBuffer[tri.idx[1]];
                    float4 c = VertexBuffer[tri.idx[2]];

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
                    // In case we exceed the maximum stack height.
                    return false;
                }
            }
        }
    }

    return false;
}