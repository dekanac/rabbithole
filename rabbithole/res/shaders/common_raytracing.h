#define SOFT_SHADOWS

layout(std430, binding = 0) readonly buffer VertexBuffer
{
	vec4 vertices[];
};

struct Triangle
{
	uint idx[3];
};

layout(std430, binding = 1) readonly buffer TrianglesBuffer
{
	Triangle triangles[];
};

layout(std430, binding = 2) readonly buffer TriangleIdxsBuffer
{
	uint triangleIndices[];
};

layout(std430, binding = 3) readonly buffer CFBVHNodesBuffer
{
	CFBVHNode cfbvhNodes[];
};

#define MAXLEN 1000.0
#define IN_SHADOW 0.0000001
#define MOLLER_TRUMBORE
#define MAX_STACK_HEIGHT 50

struct Ray
{
	vec3 origin;
	vec3 direction;
	float t;
};

struct RayHit
{
	bool hit;
	float t;
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

//Möller-Trumbore
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

	while (currStackIdx > 0)
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
					Triangle tri = triangles[triangleIndices[startIdx + i]];
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
