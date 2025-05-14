# 说明GLSL中显式栈模拟递归过程

GLSL不支持递归（GPU架构限制），因此实际代码用显式栈(nodesToVisit数组)模拟递归过程。

在下面这个代码的while循环，实现了显式栈模拟递归过程。


```glsl

bool IntersectBVH(Ray ray) {
	// if (!bvhTree.nodes) return false;
	bool hit = false;

	// 计算射线方向的倒数
	vec3 invDir = vec3(1.0 / ray.direction.x, 1.0 / ray.direction.y, 1.0 / ray.direction.z);
	
	// 计算射线方向的符号
	bool dirIsNeg[3];
	dirIsNeg[0] = (invDir.x < 0.0); 
	dirIsNeg[1] = (invDir.y < 0.0); 
	dirIsNeg[2] = (invDir.z < 0.0);
	
	// Follow ray through BVH nodes to find primitive intersections
	// 接下来射线遍历BVH节点，找到与三角形相交的点
	int toVisitOffset = 0; // 访问节点偏移量
	int currentNodeIndex = 0; // 当前节点索引
	int nodesToVisit[64]; // 访问节点栈，最多64个

	Triangle tri; // 初始化三角形
	while (true) {
		// 获取当前BVH节点
		LinearBVHNode node = getLinearBVHNode(currentNodeIndex);

		// 当前节点的包围盒
		Bound3f bound; 
		bound.pMin = node.pMin; 
		bound.pMax = node.pMax;

		// 如果射线与当前BVH节点相交
		if (IntersectBound(bound, ray, invDir, dirIsNeg)) {
			// 判断当前节点是否为叶子节点
			if (node.nPrimitives > 0) {
				// 遍历当前节点的所有三角形
				for (int i = 0; i < node.nPrimitives; ++i) {
					int offset = (node.childOffset + i); // 计算三角形在数据存储中的索引位置
					Triangle tri_t = getTriangle(offset); // 根据索引获取三角形数据
					float dis_t = hitTriangle(tri_t, ray); // 计算光线与三角形交点距离
					// 如果交点距离大于0且小于当前最小距离
					if (dis_t > 0 && dis_t < ray.hitMin) {
						ray.hitMin = dis_t; // 更新最小距离
						tri = tri_t; // 更新三角形
						hit = true; // 设置命中标志
					}
				}
				// 栈空检测
				// 当栈指针为0时表示没有待访问节点，终止循环
				if (toVisitOffset == 0) break; 
				// 从访问节点栈中获取下一个节点
				currentNodeIndex = nodesToVisit[--toVisitOffset];
			}
			else {
				// 把 BVH node 放入 _nodesToVisit_ stack, advance to near
				// 根据射线入射方向的符号，决定访问哪个子节点
				if (bool(dirIsNeg[node.axis])) {
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

	if (hit) {
		rec.Pos = ray.origin + ray.hitMin * ray.direction;
		// 我也不清楚模型的顶点坐标是顺时针还是逆时针，加负号效果是对的。
		// rec.Normal = -getTriangleNormal(tri);
		// 下面注释的glsl代码可以解决面法向量始终超外，但会影响帧率
		vec3 rawNormal = getTriangleNormal(tri);
		rec.Normal = (dot(rawNormal, -ray.direction) > 0.0) ? rawNormal : -rawNormal;
		// rec.albedo = vec3(0.83, 0.73, 0.1); 
		// rec.albedo = rec.Normal * 0.5 + 0.5; // 法线可视化
		rec.material = tri.material; 
		// rec.materialIndex = 0;
		// rec.materialIndex = tri.materialType;
	}
	return hit;
}
```

用一个具体例子说明显式栈如何模拟递归过程。假设我们有如下BVH树结构：
```txt
        A (根节点)
       / \
      B   C
     / \
    D   E
```
其中：
- A是根节点（非叶节点）
- B是左子节点（非叶节点）
- C是右子节点（叶节点）
- D和E是B的子节点（都是叶节点）

递归版本的遍历顺序会是：
- 访问A → 处理B → 处理D → 回溯到B → 处理E → 回溯到A → 处理C

显式栈版本的执行流程如下（结合代码中的nodesToVisit数组）：
```txt
// 初始状态
currentNodeIndex = 0 (A节点)
nodesToVisit = []
toVisitOffset = 0

// 第1次循环：处理A节点
发现A是非叶节点 → 根据分割轴决定访问顺序
假设先访问B节点，将C节点压栈：
nodesToVisit = [C] (栈顶在索引0)
currentNodeIndex = B的索引

// 第2次循环：处理B节点
发现B是非叶节点 → 根据分割轴决定访问顺序
假设先访问D节点，将E节点压栈：
nodesToVisit = [C, E] (栈顶在索引1)
currentNodeIndex = D的索引

// 第3次循环：处理D节点（叶节点）
与所有三角形求交 → 处理完成后检查栈
nodesToVisit = [C, E]
从栈顶弹出E节点：
currentNodeIndex = E的索引
toVisitOffset = 1 → 0

// 第4次循环：处理E节点（叶节点）
与所有三角形求交 → 处理完成后检查栈
nodesToVisit = [C]
从栈顶弹出C节点：
currentNodeIndex = C的索引
toVisitOffset = 0

// 第5次循环：处理C节点（叶节点）
与所有三角形求交 → 栈空 → 循环结束
```
关键点说明：
1. 栈的LIFO特性：最后压入的节点最先处理（与递归的"深度优先"特性一致）
2. 栈存储的是待回溯的节点：当需要先处理左子节点时，将右子节点索引压栈
3. 栈指针管理：
- toVisitOffset++ 表示压栈（写入新节点）
- nodesToVisit[--toVisitOffset] 表示弹栈（读取并删除栈顶）
4. 与递归的对应关系：
```c++
   // 递归版本隐含的栈操作
   visit(A) {
       visit(B) {
           visit(D) {}
           visit(E) {} // 相当于压栈E
       }
       visit(C) {} // 相当于压栈C
   }
```
这种显式栈实现：
- 完全模拟了递归调用的入栈/出栈过程
- 用currentNodeIndex追踪当前处理节点
- 用nodesToVisit数组存储待处理的"返回地址"（即需要回溯的节点索引）
- 通过栈深度限制（代码中的64）防止溢出，而递归版本有栈深度限制但通常隐式管理