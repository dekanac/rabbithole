#include "BVH.h"

#include <immintrin.h>
#include <glm/glm.hpp>
#include <memory>

namespace BVH
{
	void Subdivide(uint32_t nodeIdx);
	void UpdateNodeBounds(uint32_t nodeIdx);

	struct BVHNode
	{
		union { struct { glm::vec3 aabbMin; uint32_t leftFirst; }; __m128 aabbMin4; };
		union { struct { glm::vec3 aabbMax; uint32_t triCount; }; __m128 aabbMax4; };
		bool isLeaf() { return triCount > 0; }
	};

	struct AABB
	{
		glm::vec3 bmin = glm::vec3{ 1e30f }, bmax = glm::vec3{ -1e30f };
		void grow(glm::vec3 p) { bmin = glm::min(bmin, p); bmax = glm::max(bmax, p); }
		void grow(AABB& b) { if (b.bmin.x != 1e30f) { grow(b.bmin); grow(b.bmax); } }
		float area()
		{
			glm::vec3 e = bmax - bmin; // box extent
			return e.x * e.y + e.y * e.z + e.z * e.x;
		}
	};

	struct Triangle { glm::vec3 vertex0, vertex1, vertex2; glm::vec3 centroid; };
	struct Bin { BVH::AABB bounds; int triCount = 0; };

#define N	12582
#define BINS 8

	Triangle tri[N];
	uint32_t triIdx[N];
	BVH::BVHNode* bvhNode = 0;
	uint32_t rootNodeIdx = 0, nodesUsed = 2;

	void BuildBVH()
	{
		// create the BVH node pool
		bvhNode = (BVHNode*)_aligned_malloc(sizeof(BVHNode) * N * 2, 64);
		// populate triangle index array
		for (int i = 0; i < N; i++) triIdx[i] = i;
		// calculate triangle centroids for partitioning
		for (int i = 0; i < N; i++)
			tri[i].centroid = (tri[i].vertex0 + tri[i].vertex1 + tri[i].vertex2) * 0.3333f;
		// assign all triangles to root node
		BVH::BVHNode& root = bvhNode[rootNodeIdx];
		root.leftFirst = 0, root.triCount = N;
		UpdateNodeBounds(rootNodeIdx);
		// subdivide recursively
		//Timer t;
		Subdivide(rootNodeIdx);
		//printf("BVH (%i nodes) constructed in %.2fms.\n", nodesUsed, t.elapsed() * 1000);
	}

	void UpdateNodeBounds(uint32_t nodeIdx)
	{
		BVH::BVHNode& node = bvhNode[nodeIdx];
		node.aabbMin = glm::vec3(1e30f);
		node.aabbMax = glm::vec3(-1e30f);
		for (uint32_t first = node.leftFirst, i = 0; i < node.triCount; i++)
		{
			uint32_t leafTriIdx = triIdx[first + i];
			Triangle& leafTri = tri[leafTriIdx];
			node.aabbMin = glm::min(node.aabbMin, leafTri.vertex0);
			node.aabbMin = glm::min(node.aabbMin, leafTri.vertex1);
			node.aabbMin = glm::min(node.aabbMin, leafTri.vertex2);
			node.aabbMax = glm::max(node.aabbMax, leafTri.vertex0);
			node.aabbMax = glm::max(node.aabbMax, leafTri.vertex1);
			node.aabbMax = glm::max(node.aabbMax, leafTri.vertex2);
		}
	}

	float FindBestSplitPlane(BVH::BVHNode& node, int& axis, float& splitPos)
	{
		float bestCost = 1e30f;
		for (int a = 0; a < 3; a++)
		{
			float boundsMin = 1e30f, boundsMax = -1e30f;
			for (uint32_t i = 0; i < node.triCount; i++)
			{
				Triangle& triangle = tri[triIdx[node.leftFirst + i]];
				boundsMin = glm::min(boundsMin, triangle.centroid[a]);
				boundsMax = glm::max(boundsMax, triangle.centroid[a]);
			}
			if (boundsMin == boundsMax) continue;
			// populate the bins
			Bin bin[BINS];
			float scale = BINS / (boundsMax - boundsMin);
			for (uint32_t i = 0; i < node.triCount; i++)
			{
				Triangle& triangle = tri[triIdx[node.leftFirst + i]];
				int binIdx = glm::min(BINS - 1, (int)((triangle.centroid[a] - boundsMin) * scale));
				bin[binIdx].triCount++;
				bin[binIdx].bounds.grow(triangle.vertex0);
				bin[binIdx].bounds.grow(triangle.vertex1);
				bin[binIdx].bounds.grow(triangle.vertex2);
			}
			// gather data for the 7 planes between the 8 bins
			float leftArea[BINS - 1], rightArea[BINS - 1];
			int leftCount[BINS - 1], rightCount[BINS - 1];
			BVH::AABB leftBox, rightBox;
			int leftSum = 0, rightSum = 0;
			for (int i = 0; i < BINS - 1; i++)
			{
				leftSum += bin[i].triCount;
				leftCount[i] = leftSum;
				leftBox.grow(bin[i].bounds);
				leftArea[i] = leftBox.area();
				rightSum += bin[BINS - 1 - i].triCount;
				rightCount[BINS - 2 - i] = rightSum;
				rightBox.grow(bin[BINS - 1 - i].bounds);
				rightArea[BINS - 2 - i] = rightBox.area();
			}
			// calculate SAH cost for the 7 planes
			scale = (boundsMax - boundsMin) / BINS;
			for (int i = 0; i < BINS - 1; i++)
			{
				float planeCost = leftCount[i] * leftArea[i] + rightCount[i] * rightArea[i];
				if (planeCost < bestCost)
					axis = a, splitPos = boundsMin + scale * (i + 1), bestCost = planeCost;
			}
		}
		return bestCost;
	}

	float CalculateNodeCost(BVH::BVHNode& node)
	{
		glm::vec3 e = node.aabbMax - node.aabbMin; // extent of the node
		float surfaceArea = e.x * e.y + e.y * e.z + e.z * e.x;
		return node.triCount * surfaceArea;
	}

	void Subdivide(uint32_t nodeIdx)
	{
		// terminate recursion
		BVHNode& node = bvhNode[nodeIdx];
		// determine split axis using SAH
		int axis;
		float splitPos;
		float splitCost = FindBestSplitPlane(node, axis, splitPos);
		float nosplitCost = CalculateNodeCost(node);
		if (splitCost >= nosplitCost) return;
		// in-place partition
		int i = node.leftFirst;
		int j = i + node.triCount - 1;
		while (i <= j)
		{
			if (tri[triIdx[i]].centroid[axis] < splitPos)
				i++;
			else
				std::swap(triIdx[i], triIdx[j--]);
		}
		// abort split if one of the sides is empty
		int leftCount = i - node.leftFirst;
		if (leftCount == 0 || leftCount == node.triCount) return;
		// create child nodes
		int leftChildIdx = nodesUsed++;
		int rightChildIdx = nodesUsed++;
		bvhNode[leftChildIdx].leftFirst = node.leftFirst;
		bvhNode[leftChildIdx].triCount = leftCount;
		bvhNode[rightChildIdx].leftFirst = i;
		bvhNode[rightChildIdx].triCount = node.triCount - leftCount;
		node.leftFirst = leftChildIdx;
		node.triCount = 0;
		UpdateNodeBounds(leftChildIdx);
		UpdateNodeBounds(rightChildIdx);
		// recurse
		Subdivide(leftChildIdx);
		Subdivide(rightChildIdx);
	}
}