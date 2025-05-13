#pragma once
#ifndef __BVHTREE_H__
#define __BVHTREE_H__

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <tool/Shape.h>
#include <tool/Camera.h>

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
	glm::vec3 pMin, pMax;
	float nPrimitives;
	float axis;
	float childOffset; //第二个子节点位置索引 或 基元起始位置索引
};

void setBound(LinearBVHNode & lb, const Bound3f& bound) {
	lb.pMax = bound.pMax;
	lb.pMin = bound.pMin;
}

void getBound(const LinearBVHNode & lb, Bound3f& bound) {
	bound.pMax = lb.pMax;
	bound.pMin = lb.pMin;
}

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
	int nodeNum;
	int nodeNumX, nodeNumY;
	float *NodeArray;

	LinearBVHNode *nodes = nullptr;
	std::vector<std::shared_ptr<Triangle>> primitives;

	int meshNum;
	int meshNumX, meshNumY;
	float *MeshArray;

	int maxPrimsInNode = 1; // 控制叶子节点最大三角形数量的参数

	BVHTree() {}

	void releaseAll() {
		delete[] NodeArray; NodeArray = nullptr;
		delete[] MeshArray; MeshArray = nullptr;
		nodeNum = 0;
		meshNum = 0;
	}

	// 构建BVH树
	void BVHBuildTree(std::vector<std::shared_ptr<Triangle>> p, int stride = 42) {
		// 1. 数据准备阶段
		primitives = std::move(p); // 转移三角形数据所有权
		if (primitives.empty()) return;

		// Initialize primitives
		// 2. 创建基元信息数组（包含包围盒和质心）
		std::vector<BVHPrimitiveInfo> primitiveInfo(primitives.size());
		for (size_t i = 0; i < primitives.size(); ++i)
			primitiveInfo[i] = { i, getTriangleBound(*primitives[i])};

		// Build BVH tree
		// 3. 递归构建BVH树
		int totalNodes = 0;
		std::vector<std::shared_ptr<Triangle>> orderedPrims;
		orderedPrims.reserve(primitives.size());

		BVHNode *root;
		root = recursiveBuild(primitiveInfo, 0, primitives.size(),
			&totalNodes, orderedPrims);
		
		// 4. 数据重组
		primitives.swap(orderedPrims); // 用排序后的三角形替换原始数据
		primitiveInfo.resize(0); // 释放临时内存

		// 5. 展平BVH树为线性结构（便于GPU访问）
		// Compute representation of depth-first traversal of BVH tree
		nodeNum = totalNodes;
		nodes = new LinearBVHNode[totalNodes];
		int offset = 0;
		flattenBVHTree(root, &offset);

		// 6. 准备网格数据纹理
		meshNum = primitives.size();
		// int stride_t = 9 + 9 + 6 + 3 + 1;
		// int stride_t = 9 + 9 + 6;
		int stride_t = stride; // 每个三角形在纹理中的存储跨度
		int meshNumSize = meshNum * stride_t;
		float mesh_x_f = sqrtf(meshNumSize);
		meshNumX = ceilf(mesh_x_f);
		meshNumY = ceilf((float)meshNumSize / (float)meshNumX);
		std::cout << "meshNumX = " << meshNumX << " meshNumY = " << meshNumY << std::endl;

		// 7. 准备BVH节点数据纹理
		MeshArray = new float[(meshNumX * meshNumY) * stride_t];
		// 顶点赋值
		for (int i = 0; i < meshNum; i++) {
			MeshArray[i * stride_t + 0] = primitives[i]->v0.x;
			MeshArray[i * stride_t + 1] = primitives[i]->v0.y;
			MeshArray[i * stride_t + 2] = primitives[i]->v0.z;
			MeshArray[i * stride_t + 3] = primitives[i]->v1.x;
			MeshArray[i * stride_t + 4] = primitives[i]->v1.y;
			MeshArray[i * stride_t + 5] = primitives[i]->v1.z;
			MeshArray[i * stride_t + 6] = primitives[i]->v2.x;
			MeshArray[i * stride_t + 7] = primitives[i]->v2.y;
			MeshArray[i * stride_t + 8] = primitives[i]->v2.z;

			MeshArray[i * stride_t + 9] = primitives[i]->n0.x;
			MeshArray[i * stride_t + 10] = primitives[i]->n0.y;
			MeshArray[i * stride_t + 11] = primitives[i]->n0.z;
			MeshArray[i * stride_t + 12] = primitives[i]->n1.x;
			MeshArray[i * stride_t + 13] = primitives[i]->n1.y;
			MeshArray[i * stride_t + 14] = primitives[i]->n1.z;
			MeshArray[i * stride_t + 15] = primitives[i]->n2.x;
			MeshArray[i * stride_t + 16] = primitives[i]->n2.y;
			MeshArray[i * stride_t + 17] = primitives[i]->n2.z;

			MeshArray[i * stride_t + 18] = primitives[i]->u0.x;
			MeshArray[i * stride_t + 19] = primitives[i]->u0.y;
			MeshArray[i * stride_t + 20] = primitives[i]->u1.x;
			MeshArray[i * stride_t + 21] = primitives[i]->u1.y;
			MeshArray[i * stride_t + 22] = primitives[i]->u2.x;
			MeshArray[i * stride_t + 23] = primitives[i]->u2.y;

			MeshArray[i * stride_t + 24] = primitives[i]->material.emissive.x;
			MeshArray[i * stride_t + 25] = primitives[i]->material.emissive.y;
			MeshArray[i * stride_t + 26] = primitives[i]->material.emissive.z;

			MeshArray[i * stride_t + 27] = primitives[i]->material.baseColor.x;
			MeshArray[i * stride_t + 28] = primitives[i]->material.baseColor.y;
			MeshArray[i * stride_t + 29] = primitives[i]->material.baseColor.z;

			MeshArray[i * stride_t + 30] = primitives[i]->material.subsurface;
			MeshArray[i * stride_t + 31] = primitives[i]->material.metallic;
			MeshArray[i * stride_t + 32] = primitives[i]->material.specular;
			MeshArray[i * stride_t + 33] = primitives[i]->material.specularTint;
			MeshArray[i * stride_t + 34] = primitives[i]->material.roughness;
			MeshArray[i * stride_t + 35] = primitives[i]->material.anisotropic;
			MeshArray[i * stride_t + 36] = primitives[i]->material.sheen;
			MeshArray[i * stride_t + 37] = primitives[i]->material.sheenTint;
			MeshArray[i * stride_t + 38] = primitives[i]->material.clearcoat;
			MeshArray[i * stride_t + 39] = primitives[i]->material.clearcoatGloss;
			MeshArray[i * stride_t + 40] = primitives[i]->material.IOR;
			MeshArray[i * stride_t + 41] = primitives[i]->material.transmission;
		}

		int nodeNumSize = nodeNum * (9);
		float Node_x_f = sqrtf(nodeNumSize);
		nodeNumX = ceilf(Node_x_f);
		nodeNumY = ceilf((float)nodeNumSize / (float)nodeNumX);
		std::cout << "nodeNumX = " << nodeNumX << " nodeNumY = " << nodeNumY << std::endl;

		NodeArray = new float[(nodeNumX * nodeNumY)];
		for (int i = 0; i < nodeNum; i++) {
			NodeArray[i * (9) + 0] = nodes[i].pMin.x;
			NodeArray[i * (9) + 1] = nodes[i].pMin.y;
			NodeArray[i * (9) + 2] = nodes[i].pMin.z;
			NodeArray[i * (9) + 3] = nodes[i].pMax.x;
			NodeArray[i * (9) + 4] = nodes[i].pMax.y;
			NodeArray[i * (9) + 5] = nodes[i].pMax.z;

			//std::cout << nodes[i].pMin.x << " " << nodes[i].pMin.y << " " << nodes[i].pMin.z << " " <<	nodes[i].pMax.x << " " << nodes[i].pMax.y << " " << nodes[i].pMax.z << " " << std::endl;

			NodeArray[i * (9) + 6] = nodes[i].nPrimitives;
			NodeArray[i * (9) + 7] = nodes[i].axis;
			NodeArray[i * (9) + 8] = nodes[i].childOffset;
		}

		// 8. 清理临时节点数据
		delete[] nodes;
		nodes = nullptr;

	}

	// 递归构建BVH树
	BVHNode *recursiveBuild(std::vector<BVHPrimitiveInfo> &primitiveInfo,
							int start, int end, int *totalNodes,
							std::vector<std::shared_ptr<Triangle>> &orderedPrims) 
	{
		BVHNode* node = new BVHNode;
		(*totalNodes)++;
		// 计算BVH节点中所有基元的边界
		Bound3f bounds;
		for (int i = start; i < end; ++i)
			bounds = Union(bounds, primitiveInfo[i].bound);

		int nPrimitives = end - start;
		
		// 当前硬编码为1个三角形就停止划分
		if (nPrimitives == maxPrimsInNode) {
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
				int mid = ((start + end) / 2);

				std::nth_element(&primitiveInfo[start], 
								 &primitiveInfo[mid],
					             &primitiveInfo[end - 1] + 1,
					[dim](const BVHPrimitiveInfo &a, const BVHPrimitiveInfo &b)
					{
						return a.centroid[dim] < b.centroid[dim];
					}
				);

				// 递归构建子节点
				BVHNode* left = recursiveBuild(primitiveInfo, start, mid, totalNodes, orderedPrims);
				BVHNode* right = recursiveBuild(primitiveInfo, mid, end, totalNodes, orderedPrims);
				node->InitInterior(dim, left, right);

				return node;
			}
		}
	}

	int flattenBVHTree(BVHNode *node, int *offset) {
		LinearBVHNode *linearNode = &nodes[*offset];
		setBound(*linearNode, node->bound);
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
	// if (!bvhTree.nodes) return false;
	bool hit = false;

	glm::vec3 invDir(1 / ray.direction.x, 1 / ray.direction.y, 1 / ray.direction.z);
	int dirIsNeg[3] = { invDir.x < 0, invDir.y < 0, invDir.z < 0 };
	// Follow ray through BVH nodes to find primitive intersections
	int toVisitOffset = 0, currentNodeIndex = 0;
	int nodesToVisit[64];
	while (true) {
		int offset1 = currentNodeIndex * (9);
		LinearBVHNode node;
		node.pMin = glm::vec3(bvhTree.NodeArray[offset1 + 0], bvhTree.NodeArray[offset1 + 1], bvhTree.NodeArray[offset1 + 2]);
		node.pMax = glm::vec3(bvhTree.NodeArray[offset1 + 3], bvhTree.NodeArray[offset1 + 4], bvhTree.NodeArray[offset1 + 5]);
		node.nPrimitives = bvhTree.NodeArray[offset1 + 6];
		node.axis = bvhTree.NodeArray[offset1 + 7];
		node.childOffset = bvhTree.NodeArray[offset1 + 8];

		// Ray 与 BVH的交点
		Bound3f bound;
		getBound(node, bound);
		if (IntersectBound(bound, ray, invDir, dirIsNeg)) {
			if (node.nPrimitives > 0) {
				// Ray 与 叶节点的交点
				for (int i = 0; i < node.nPrimitives; ++i) {
					int offset = (node.childOffset + i) * (9 + 9 + 6);
					Triangle tri; 
					tri.v0 = glm::vec3(bvhTree.MeshArray[offset + 0], bvhTree.MeshArray[offset + 1], bvhTree.MeshArray[offset + 2]);
					tri.v1 = glm::vec3(bvhTree.MeshArray[offset + 3], bvhTree.MeshArray[offset + 4], bvhTree.MeshArray[offset + 5]);
					tri.v2 = glm::vec3(bvhTree.MeshArray[offset + 6], bvhTree.MeshArray[offset + 7], bvhTree.MeshArray[offset + 8]);
					float t = hitTriangle(tri, ray);
					if (t > 0.0f) hit = true; 
				}
				if (toVisitOffset == 0) break;
				currentNodeIndex = nodesToVisit[--toVisitOffset];
			}
			else {
				// 把 BVH node 放入 _nodesToVisit_ stack, advance to near
				if (dirIsNeg[int(node.axis)]) {
					nodesToVisit[toVisitOffset++] = currentNodeIndex + 1;
					currentNodeIndex = node.childOffset;
				}
				else {
					nodesToVisit[toVisitOffset++] = node.childOffset;
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


#define STB_IMAGE_WRITE_IMPLEMENTATION // include之前必须定义
#include <tool/stb_image_write.h>>


void BVHTest(const BVHTree& bvhTree, const Camera& camera) {

	Ray cameraRay;
	cameraRay.origin = camera.Position;
	
	int width = 120, height = 80;
	unsigned char * data = new unsigned char[width * height * 4];

	for (int j = 0; j < height; j++) {
		 for (int i = 0; i < width; i++) {
			float x = (float)i / (float)width;
			float y = (float)j / (float)height;
			cameraRay.direction = 
				normalize(camera.LeftBottomCorner
				 		+ (x * 2.0f * camera.halfW) * camera.Right
				  		+ (y * 2.0f * camera.halfH) * camera.Up
				  );
			
			hitRecord rec;
			if (IntersectBVH(bvhTree, cameraRay, rec)) 
				data[(i + (height - j - 1) * width) * 4 + 0] = 255;
			else 
				data[(i + (height - j - 1) * width) * 4 + 0] = 0;

			data[(i + (height - j - 1) * width) * 4 + 1] = 0;
			data[(i + (height - j - 1) * width) * 4 + 2] = 0;
			data[(i + (height - j - 1) * width) * 4 + 3] = 255;

			//std::cout << "(" << i <<", " << j << ")" << std::endl;
		}
	}

	stbi_write_png("Test.png", width, height, 4, data, 4 * width);
	
	delete[] data;
}


#endif


