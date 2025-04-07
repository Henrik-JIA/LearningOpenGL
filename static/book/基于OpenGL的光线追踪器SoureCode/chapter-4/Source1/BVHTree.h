#pragma once
#ifndef __BVHTREE_H__
#define __BVHTREE_H__

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "../Source/Shape.h"
#include "../Source/Camera.h"

#include <algorithm>
#include <vector>
#include <memory>
#include <iostream>

int totalPrimitives = 0;

// 基本数据结构

struct BVHNode {
	BVHNode * children[2];
	int splitAxis, firstPrimOffset, nPrimitives;
	Bound3f bound;
	// 初始化为叶节点
	void InitLeaf(int first, int n, const Bound3f &b) {
		firstPrimOffset = first;
		nPrimitives = n;
		bound = b;
		children[0] = children[1] = nullptr;	
		totalPrimitives += n;
	}
	// 初始化为内部节点
	void InitInterior(int axis, BVHNode *c0, BVHNode *c1) {
		children[0] = c0;
		children[1] = c1;
		bound = Union(c0->bound, c1->bound);
		splitAxis = axis;
		nPrimitives = 0;
	}
};

struct LinearBVHNode {
	Bound3f bound;
	int nPrimitives;
	int axis;
	int childOffset; //第二个子节点位置索引 或 基元起始位置索引
};

struct BVHPrimitiveInfo {
	BVHPrimitiveInfo() {}
	BVHPrimitiveInfo(size_t primitiveNumber, const Bound3f &bounds)
		: primitiveNumber(primitiveNumber),
		bound(bounds),
		centroid(.5f * bounds.pMin + .5f * bounds.pMax) {}
	size_t primitiveNumber;
	Bound3f bound;
	glm::vec3 centroid;
};

struct BucketInfo {
	int count = 0;
	Bound3f bounds;
};


// 构建BVH树

class BVHTree {
public:
	LinearBVHNode *nodes = nullptr;
	std::vector<std::shared_ptr<Triangle>> primitives;
	int maxPrimsInNode = 1;

	BVHTree() {}

	void BVHBuildTree(std::vector<std::shared_ptr<Triangle>> p) {
		primitives = std::move(p);
		if (primitives.empty()) return;
		// Initialize primitives
		std::vector<BVHPrimitiveInfo> primitiveInfo(primitives.size());
		for (size_t i = 0; i < primitives.size(); ++i)
			primitiveInfo[i] = { i, getTriangleBound(*primitives[i])};

		// Build BVH tree
		int totalNodes = 0;
		std::vector<std::shared_ptr<Triangle>> orderedPrims;
		orderedPrims.reserve(primitives.size());

		BVHNode *root;
		root = recursiveBuild(primitiveInfo, 0, primitives.size(),
			&totalNodes, orderedPrims);
		primitives.swap(orderedPrims);
		primitiveInfo.resize(0);

		//std::cout << "totalNodes:" << totalNodes << std::endl;

		// Compute representation of depth-first traversal of BVH tree
		nodes = new LinearBVHNode[totalNodes];
		int offset = 0;
		flattenBVHTree(root, &offset);
	}

	BVHNode *recursiveBuild(std::vector<BVHPrimitiveInfo> &primitiveInfo,
		int start, int end, int *totalNodes,
		std::vector<std::shared_ptr<Triangle>> &orderedPrims) {

		BVHNode* node = new BVHNode;
		(*totalNodes)++;
		// 计算BVH节点中所有基元的边界
		Bound3f bounds;
		for (int i = start; i < end; ++i)
			bounds = Union(bounds, primitiveInfo[i].bound);
		int nPrimitives = end - start;
		if (nPrimitives == 1) {
			// 构建叶节点
			int firstPrimOffset = orderedPrims.size();
			for (int i = start; i < end; ++i) {
				int primNum = primitiveInfo[i].primitiveNumber;
				orderedPrims.push_back(primitives[primNum]);
			}
			node->InitLeaf(firstPrimOffset, nPrimitives, bounds);
			return node;
		}
		else {
			// 首先计算基元的边界，选择用于划分的维度
			Bound3f centroidBounds;
			for (int i = start; i < end; ++i)
				centroidBounds = Union(centroidBounds, primitiveInfo[i].centroid);
			int dim = centroidBounds.MaximumExtent();

			// 把基元划分到两个子集，构建子节点
			int mid = (start + end) / 2;
			if (centroidBounds.pMax[dim] == centroidBounds.pMin[dim]) {
				// 构建叶节点
				int firstPrimOffset = orderedPrims.size();
				for (int i = start; i < end; ++i) {
					int primNum = primitiveInfo[i].primitiveNumber;
					orderedPrims.push_back(primitives[primNum]);
				}
				node->InitLeaf(firstPrimOffset, nPrimitives, bounds);
				return node;
			}
			else {
				// 基于split方法将基元划分为两部分
				{
					mid = (start + end) / 2;
					std::nth_element(&primitiveInfo[start], &primitiveInfo[mid],
						&primitiveInfo[end - 1] + 1,
						[dim](const BVHPrimitiveInfo &a, const BVHPrimitiveInfo &b) {
						return a.centroid[dim] < b.centroid[dim];
					});
				}
				node->InitInterior(dim,
					recursiveBuild(primitiveInfo, start, mid,
						totalNodes, orderedPrims),
					recursiveBuild(primitiveInfo, mid, end,
						totalNodes, orderedPrims));
			}
		}
	}

	int flattenBVHTree(BVHNode *node, int *offset) {
		LinearBVHNode *linearNode = &nodes[*offset];
		linearNode->bound = node->bound;
		int myOffset = (*offset)++;
		if (node->nPrimitives > 0) {
			linearNode->childOffset = node->firstPrimOffset;
			linearNode->nPrimitives = node->nPrimitives;
		}
		else {
			// Create interior flattened BVH node
			linearNode->axis = node->splitAxis;
			linearNode->nPrimitives = 0;
			flattenBVHTree(node->children[0], offset);
			linearNode->childOffset = flattenBVHTree(node->children[1], offset);
		}
		return myOffset;
	}

};

struct hitRecord {
	glm::vec3 Pos;
	glm::vec3 Normal;
};
bool IntersectBVH(const BVHTree& bvhTree, const Ray &ray, hitRecord& rec) {
	if (!bvhTree.nodes) return false;
	bool hit = false;

	glm::vec3 invDir(1 / ray.direction.x, 1 / ray.direction.y, 1 / ray.direction.z);
	int dirIsNeg[3] = { invDir.x < 0, invDir.y < 0, invDir.z < 0 };
	// Follow ray through BVH nodes to find primitive intersections
	int toVisitOffset = 0, currentNodeIndex = 0;
	int nodesToVisit[64];
	while (true) {

		const LinearBVHNode *node = &bvhTree.nodes[currentNodeIndex];
		// Ray 与 BVH的交点
		if (IntersectBound(node->bound, ray, invDir, dirIsNeg)) {
			if (node->nPrimitives > 0) {

				// Ray 与 叶节点的交点
				for (int i = 0; i < node->nPrimitives; ++i) {
					float t = hitTriangle(*bvhTree.primitives[node->childOffset + i], ray);
					if (t > 0.0f) hit = true; 
				}
				if (toVisitOffset == 0) break;
				currentNodeIndex = nodesToVisit[--toVisitOffset];
			}
			else {
				// 把 BVH node 放入 _nodesToVisit_ stack, advance to near
				if (dirIsNeg[node->axis]) {
					nodesToVisit[toVisitOffset++] = currentNodeIndex + 1;
					currentNodeIndex = node->childOffset;
				}
				else {
					nodesToVisit[toVisitOffset++] = node->childOffset;
					currentNodeIndex = currentNodeIndex + 1;
				}
			}
		}
		else {
			if (toVisitOffset == 0) break;
			currentNodeIndex = nodesToVisit[--toVisitOffset];
		}
	}
	return hit;
}


#define STB_IMAGE_WRITE_IMPLEMENTATION	// include之前必须定义
#include "../Tool/stb_image_write.h"
void BVHTest(const BVHTree& bvhTree, const Camera& camera) {

	Ray cameraRay;
	cameraRay.origin = camera.cameraPos;
	
	int width = 240, height = 160;
	unsigned char * data = new unsigned char[width * height * 4];

	for (int j = 0; j < height; j++) {
		 for (int i = 0; i < width; i++) {
			float x = (float)i / (float)width;
			float y = (float)j / (float)height;
			cameraRay.direction = 
				normalize(camera.LeftBottomCorner + (x * 2.0f * camera.halfW) * camera.cameraRight + (y * 2.0f * camera.halfH) * camera.cameraUp);
			
			hitRecord rec;
			if (IntersectBVH(bvhTree, cameraRay, rec)) 
				data[(i + (height - j - 1) * width) * 4 + 0] = 255;
			else 
				data[(i + (height - j - 1) * width) * 4 + 0] = 0;

			data[(i + (height - j - 1) * width) * 4 + 1] = 0;
			data[(i + (height - j - 1) * width) * 4 + 2] = 0;
			data[(i + (height - j - 1) * width) * 4 + 3] = 255;

			std::cout << "(" << i <<", " << j << ")" << std::endl;
		}
	}

	stbi_write_png("Test.png", width, height, 4, data, 4 * width);
	
	delete[] data;
}


#endif


