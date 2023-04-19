#pragma once

#include "common.h"

#include <list>
#include <vector>

namespace BVH 
{
	struct Triangle
	{
		int indices[3];
	};

	struct BVHNode
	{
		rabbitVec3f bottom;
		rabbitVec3f top;
		virtual bool IsLeaf() = 0;
	};

	struct BVHInner : BVHNode
	{
		BVHNode* left;
		BVHNode* right;
		virtual bool IsLeaf() { return false; }
	};

	struct BVHLeaf : BVHNode
	{
		std::list<const Triangle*> triangles;
		virtual bool IsLeaf() { return true; }
	};

	struct BBoxTmp
	{
		rabbitVec3f bottom;
		rabbitVec3f top;
		rabbitVec3f center;

		const Triangle* pTri;

		BBoxTmp()
			: bottom(FLT_MAX, FLT_MAX, FLT_MAX)
			, top(-FLT_MAX, -FLT_MAX, -FLT_MAX)
			, pTri(NULL)
		{}
	};
	typedef std::vector<BBoxTmp> BBoxEntries;  // vector of triangle bounding boxes needed during BVH construction

	BVHNode* Recurse(BBoxEntries& work, int depth = 0);
	BVHNode* CreateBVH(std::vector<rabbitVec4f>& vertices, std::vector<Triangle>& triangles);

	struct CacheFriendlyBVHNode
	{
		// bounding box
		rabbitVec3f bottom;
		rabbitVec3f top;

		// parameters for leafnodes and innernodes occupy same space (union) to save memory
		// top bit discriminates between leafnode and innernode
		// no pointers, but indices (int): faster

		union
		{
			// inner node - stores indexes to array of CacheFriendlyBVHNode
			struct
			{
				uint32_t idxLeft;
				uint32_t idxRight;
			} inner;
			// leaf node: stores triangle count and starting index in triangle list
			struct
			{
				uint32_t count; // Top-most bit set, leafnode if set, innernode otherwise
				uint32_t startIndexInTriIndexList;
			} leaf;
		} u;
	};

	// The ugly, cache-friendly form of the BVH: 32 bytes
	void CreateCFBVH(const Triangle* triangles, BVHNode* rootBVH, uint32_t** triIndexList, uint32_t* triIndexListNum, CacheFriendlyBVHNode** nodeList, uint32_t* nodeListNum);
	int CountBoxes(BVHNode* root);
	unsigned CountTriangles(BVHNode* root);
	void CountDepth(BVHNode* root, int depth, int& maxDepth);
	void PopulateCacheFriendlyBVH(const Triangle* pFirstTriangle, BVHNode* root, unsigned& idxBoxes, unsigned& idxTriList, uint32_t* triIndexList, CacheFriendlyBVHNode* nodeList);
}

