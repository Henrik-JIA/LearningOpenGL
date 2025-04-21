#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

#define PI 3.1415926
#define INF 114514.0
#define SIZE_BVHNODE 4
#define SIZE_TRIANGLE 12
#define RussianRoulette 0.8
#define EPSILON 0.00001

uniform int screenWidth;
uniform int screenHeight;

struct Camera {
	vec3 camPos;
	vec3 front;
	vec3 right;
	vec3 up;
	float halfH;
	float halfW;
	vec3 leftbottom;
	int LoopNum;
};
uniform Camera camera;

struct Ray {
	vec3 origin;
	vec3 direction;
	float hitMin;
};

uniform float randOrigin;
uint wseed;
float rand(void);

struct Bound3f {
	vec3 pMin, pMax;
};

struct LinearBVHNode {
	vec3 pMin, pMax;
	int nPrimitives;
	int axis;
	int childOffset; //第二个子节点位置索引 或 基元起始位置索引
};

struct Sphere {
	vec3 center;
	float radius;
	vec3 albedo;
	int materialIndex;
};
uniform Sphere sphere[4];

struct Material {
    vec3 emissive;
    vec3 baseColor;
    float subsurface;
    float metallic;
    float specular;
    float specularTint; 
    float roughness;
    float anisotropic;
    float sheen;
    float sheenTint;
    float clearcoat;
    float clearcoatGloss;
    float IOR;
    int transmission;
};

struct Triangle {
	vec3 v0, v1, v2;
	vec3 n0, n1, n2;
	vec2 u0, u1, u2;
	Material material;
};
uniform Triangle triFloor[2];

uniform sampler2D texMesh;
uniform int meshNum;
uniform sampler2D texBvhNode;
uniform int bvhNodeNum;

struct hitRecord {
	vec3 Normal;
	vec3 Pos;
	Material material; 
	float rayHitMin;
};
hitRecord rec;

struct HitResult {
    bool isHit;             // 是否命中
    bool isInside;          // 是否从内部命中
    float distance;         // 与交点的距离
    vec3 hitPoint;          // 光线命中点
    vec3 normal;            // 命中点法线
    vec3 viewDir;           // 击中该点的光线的方向
    Material material;      // 命中点的表面材质
};


float At(sampler2D dataTex, float index);
// 返回值：ray到球交点的距离
float hitSphere(Sphere s, Ray r);
float hitTriangle(Triangle tri, Ray r);
// HitResult hitWorld(Ray r); // 修改为返回HitResult
bool hitWorld(Ray r);
// vec3 shade(HitResult hit_obj, vec3 wo); 
vec3 shading(Ray r);
vec3 getTriangleNormal(Triangle tri);

bool IntersectBound(Bound3f bounds, Ray ray, vec3 invDir, bool dirIsNeg[3]);

// 采样历史帧的纹理采样器
uniform sampler2D historyTexture;

void main() {
	wseed = uint(randOrigin * float(6.95857) * (TexCoords.x * TexCoords.y));
	//if (distance(TexCoords, vec2(0.5, 0.5)) < 0.4)
	//	FragColor = vec4(rand(), rand(), rand(), 1.0);
	//else
	//	FragColor = vec4(0.0, 0.0, 0.0, 1.0);

	// 获取历史帧信息
	vec3 hist = texture(historyTexture, TexCoords).rgb;

	Ray cameraRay;
	cameraRay.origin = camera.camPos;
	cameraRay.direction = normalize(camera.leftbottom + (TexCoords.x * 2.0 * camera.halfW) * camera.right + (TexCoords.y * 2.0 * camera.halfH) * camera.up);
	cameraRay.hitMin = 100000.0;

    // // 获取第一次碰撞结果
    // HitResult firstHit = hitWorld(cameraRay);
    // // 基于碰撞结果决定颜色
    // vec3 curColor;
    // if (firstHit.isHit) {
    //     curColor = shade(firstHit, -cameraRay.direction);
    // } else {
    //     // 背景色处理
    //     float t = 0.5 * (cameraRay.direction.y + 1.0);
    //     curColor = (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
    // }


	vec3 curColor = shading(cameraRay);
	
	curColor = (1.0 / float(camera.LoopNum))*curColor + (float(camera.LoopNum - 1) / float(camera.LoopNum)) * hist;
	FragColor = vec4(curColor, 1.0);

}


// ************ 随机数功能 ************** //
float randcore(uint seed) {
	seed = (seed ^ uint(61)) ^ (seed >> uint(16));
	seed *= uint(9);
	seed = seed ^ (seed >> uint(4));
	seed *= uint(0x27d4eb2d);
	wseed = seed ^ (seed >> uint(15));
	return float(wseed) * (1.0 / 4294967296.0);
}
float rand() {
	return randcore(wseed);
}


// ********* 击中场景的相关函数 ********* // 

LinearBVHNode getLinearBVHNode(int offset) {
	int offset1 = offset * (9);
	LinearBVHNode node;
	node.pMin = vec3(At(texBvhNode, float(offset1 + 0)), At(texBvhNode, float(offset1 + 1)), At(texBvhNode, float(offset1 + 2)));
	node.pMax = vec3(At(texBvhNode, float(offset1 + 3)), At(texBvhNode, float(offset1 + 4)), At(texBvhNode, float(offset1 + 5)));
	node.nPrimitives = int(At(texBvhNode, float(offset1 + 6)));
	node.axis = int(At(texBvhNode, float(offset1 + 7)));
	node.childOffset = int(At(texBvhNode, float(offset1 + 8)));
	return node;
}

// 返回值：ray到球交点的距离
float hitSphere(Sphere s, Ray r) {
	vec3 oc = r.origin - s.center;
	float a = dot(r.direction, r.direction);
	float b = 2.0 * dot(oc, r.direction);
	float c = dot(oc, oc) - s.radius * s.radius;
	float discriminant = b * b - 4 * a * c;
	if (discriminant > 0.0) {
		float dis = (-b - sqrt(discriminant)) / (2.0 * a);
		if (dis > 0.0) return dis - 0.00001;
		else return -1.0;
	}
	else return -1.0;
}
// 返回值：ray到三角形交点的距离
float hitTriangle(Triangle tri, Ray r) {
	// 找到三角形所在平面法向量
	vec3 A = tri.v1 - tri.v0;
	vec3 B = tri.v2 - tri.v0;
	vec3 N = normalize(cross(A, B));
	// Ray与平面平行，没有交点
	if (dot(N, r.direction) == 0) return -1.0;
	float D = -dot(N, tri.v0);
	float t = -(dot(N, r.origin) + D) / dot(N, r.direction);
	if (t < 0) return -1.0;
	// 计算交点
	vec3 pHit = r.origin + t * r.direction;
	vec3 edge0 = tri.v1 - tri.v0;
	vec3 C0 = pHit - tri.v0;
	if (dot(N, cross(edge0, C0)) < 0) return -1.0;
	vec3 edge1 = tri.v2 - tri.v1;
	vec3 C1 = pHit - tri.v1;
	if (dot(N, cross(edge1, C1)) < 0) return -1.0;
	vec3 edge2 = tri.v0 - tri.v2;
	vec3 C2 = pHit - tri.v2;
	if (dot(N, cross(edge2, C2)) < 0) return -1.0;
	// 光线与Ray相交
	return t - 0.000001;
}

Triangle getTriangle(int index);
bool IntersectBVH(Ray ray) {
	// if (!bvhTree.nodes) return false;
	bool hit = false;

	vec3 invDir = vec3(1.0 / ray.direction.x, 1.0 / ray.direction.y, 1.0 / ray.direction.z);
	bool dirIsNeg[3];
	dirIsNeg[0] = (invDir.x < 0.0); dirIsNeg[1] = (invDir.y < 0.0); dirIsNeg[2] = (invDir.z < 0.0);
	// Follow ray through BVH nodes to find primitive intersections
	int toVisitOffset = 0, currentNodeIndex = 0;
	int nodesToVisit[64];

	Triangle tri;
	while (true) {
		LinearBVHNode node = getLinearBVHNode(currentNodeIndex);
		// Ray 与 BVH的交点
		Bound3f bound; bound.pMin = node.pMin; bound.pMax = node.pMax;
		if (IntersectBound(bound, ray, invDir, dirIsNeg)) {
			if (node.nPrimitives > 0) {
				// Ray 与 叶节点的交点
				for (int i = 0; i < node.nPrimitives; ++i) {
					int offset = (node.childOffset + i);
					Triangle tri_t = getTriangle(offset);
					float dis_t = hitTriangle(tri_t, ray);
					if (dis_t > 0 && dis_t < ray.hitMin) {
						ray.hitMin = dis_t;
						tri = tri_t;
						hit = true;
					}
				}
				if (toVisitOffset == 0) break;
				currentNodeIndex = nodesToVisit[--toVisitOffset];
			}
			else {
				// 把 BVH node 放入 _nodesToVisit_ stack, advance to near
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

float At(sampler2D dataTex, float index) {
	float row = (index + 0.5) / textureSize(dataTex, 0).x;
	float y = (int(row) + 0.5) / textureSize(dataTex, 0).y;
	float x = (index + 0.5 - int(row) * textureSize(dataTex, 0).x) / textureSize(dataTex, 0).x;
	vec2 texCoord = vec2(x, y);
	return texture2D(dataTex, texCoord).x;
}

Triangle getTriangle(int index) {
	Triangle tri_t;
	int offset = index * (9 + 9 + 6 + 3 + 3 + 12);
	tri_t.v0.x = At(texMesh, float(offset));
	tri_t.v0.y = At(texMesh, float(offset + 1));
	tri_t.v0.z = At(texMesh, float(offset + 2));
	tri_t.v1.x = At(texMesh, float(offset + 3));
	tri_t.v1.y = At(texMesh, float(offset + 4));
	tri_t.v1.z = At(texMesh, float(offset + 5));
	tri_t.v2.x = At(texMesh, float(offset + 6));
	tri_t.v2.y = At(texMesh, float(offset + 7));
	tri_t.v2.z = At(texMesh, float(offset + 8));

	tri_t.n0.x = At(texMesh, float(offset + 9));
	tri_t.n0.y = At(texMesh, float(offset + 10));
	tri_t.n0.z = At(texMesh, float(offset + 11));
	tri_t.n1.x = At(texMesh, float(offset + 12));
	tri_t.n1.y = At(texMesh, float(offset + 13));
	tri_t.n1.z = At(texMesh, float(offset + 14));
	tri_t.n2.x = At(texMesh, float(offset + 15));
	tri_t.n2.y = At(texMesh, float(offset + 16));
	tri_t.n2.z = At(texMesh, float(offset + 17));

	tri_t.u0.x = At(texMesh, float(offset + 18));
	tri_t.u0.y = At(texMesh, float(offset + 19));
	tri_t.u1.x = At(texMesh, float(offset + 20));
	tri_t.u1.y = At(texMesh, float(offset + 21));
	tri_t.u2.x = At(texMesh, float(offset + 22));
	tri_t.u2.y = At(texMesh, float(offset + 23));

	tri_t.material.emissive.r = At(texMesh, float(offset + 24));
	tri_t.material.emissive.g = At(texMesh, float(offset + 25));
	tri_t.material.emissive.b = At(texMesh, float(offset + 26));

	tri_t.material.baseColor.r = At(texMesh, float(offset + 27));
	tri_t.material.baseColor.g = At(texMesh, float(offset + 28));
	tri_t.material.baseColor.b = At(texMesh, float(offset + 29));

	tri_t.material.subsurface = At(texMesh, float(offset + 30));
	tri_t.material.metallic = At(texMesh, float(offset + 31));
	tri_t.material.specular = At(texMesh, float(offset + 32));
	tri_t.material.specularTint = At(texMesh, float(offset + 33));
	tri_t.material.roughness = At(texMesh, float(offset + 34));
	tri_t.material.anisotropic = At(texMesh, float(offset + 35));
	tri_t.material.sheen = At(texMesh, float(offset + 36));
	tri_t.material.sheenTint = At(texMesh, float(offset + 37));
	tri_t.material.clearcoat = At(texMesh, float(offset + 38));
	tri_t.material.clearcoatGloss = At(texMesh, float(offset + 39));
	tri_t.material.IOR = At(texMesh, float(offset + 40));
	tri_t.material.transmission = int(At(texMesh, float(offset + 41)));

	return tri_t;
}

vec3 getTriangleNormal(Triangle tri) {
	return normalize(cross(tri.v2 - tri.v0, tri.v1 - tri.v0));
}

// // 修改hitWorld函数，返回HitResult而不是boolean
// HitResult hitWorld(Ray r) {
//     HitResult res;
//     res.isHit = false;
//     res.distance = 100000.0;
//     res.isInside = false;
    
//     // 原来的碰撞检测逻辑
//     bool ifHitSphere = false;
//     bool ifHitTriangleMesh = false;
//     bool ifHitTriangleFloor = false;
//     int hitSphereIndex;
//     int hitTriangleIndex;
// 	// float hitDistance = 100000.0;

//     // 先保存BVH相交信息，但不立即返回
//     if (IntersectBVH(r)) {
//         ifHitTriangleMesh = true;
//         // 将结果保存到临时变量，后面比较谁更近
//         float meshDistance = r.hitMin;
//         vec3 meshHitPoint = rec.Pos;
//         vec3 meshNormal = rec.Normal;
//         Material meshMaterial = rec.material;
        
//         // 更新结果信息
//         if (meshDistance < res.distance) {
//             res.isHit = true;
//             res.distance = meshDistance;
//             res.hitPoint = meshHitPoint;
//             res.normal = meshNormal;
//             res.material = meshMaterial;
//             res.viewDir = r.direction;
//         }
//     }

//     // 检测地板是否相交
//     for (int i = 0; i < 2; i++) {
//         float dis_t = hitTriangle(triFloor[i], r);
//         // 只有当地板相交点比当前已知的最近相交点更近时才更新
//         if (dis_t > 0 && dis_t < res.distance) {
//             res.isHit = true;
//             res.distance = dis_t; // 更新为地板的碰撞距离
//             res.hitPoint = r.origin + dis_t * r.direction; // 使用dis_t计算碰撞点
//             res.normal = getTriangleNormal(triFloor[hitTriangleIndex]);
//             // 设置地板材质属性
//             res.material.baseColor = vec3(0.8, 0.8, 0.8);
//             res.material.emissive = vec3(0.0);
//             res.material.roughness = 0.1;
//             res.material.transmission = 1;
//             res.viewDir = r.direction;
//         }
//     }

//     if (ifHitSphere) {
//         // res.isHit = true;
//         // res.distance = hitDistance;
//         // res.hitPoint = r.origin + hitDistance * r.direction;
//         // res.normal = normalize(res.hitPoint - sphere[hitSphereIndex].center);
//         // res.material.baseColor = sphere[hitSphereIndex].albedo;
//         // res.material.transmission = sphere[hitSphereIndex].materialIndex;
//         // res.viewDir = r.direction;
//     }
//     else if (ifHitTriangleFloor) {
//         res.isHit = true;
//         res.distance = r.hitMin;
//         res.hitPoint = r.origin + r.hitMin * r.direction;
//         res.normal = getTriangleNormal(triFloor[hitTriangleIndex]);
//         // 设置完整的材质属性
//         res.material.baseColor = vec3(0.8, 0.8, 0.8);
//         res.material.emissive = vec3(0.0);
//         res.material.roughness = 0.1; // 光滑的金属地板
//         res.material.transmission = 1;
//         res.viewDir = r.direction;
//     }
//     else if (ifHitTriangleMesh) {
//         res.isHit = true;
//         res.distance = r.hitMin;
//         res.hitPoint = rec.Pos;
//         res.normal = rec.Normal;
//         res.material = rec.material;
//         res.viewDir = r.direction;
//     }
    
//     return res;
// }

// 返回值：ray到球交点的距离
bool hitWorld(Ray r) {

	bool ifHitSphere = false;
	bool ifHitTriangleMesh = false;
	bool ifHitTriangleFloor = false;
	int hitSphereIndex;
	int hitTriangleIndex;

	if (IntersectBVH(r)) {
		r.hitMin = rec.rayHitMin;
		ifHitTriangleMesh = true;
	}

	// 计算地板相交
	for (int i = 0; i < 2; i++) {
		float dis_t = hitTriangle(triFloor[i], r);
		if (dis_t > 0 && dis_t < r.hitMin) {
			r.hitMin = dis_t;
			hitTriangleIndex = i;
			ifHitTriangleFloor = true;
		}
	}

	// 计算球
	/*for (int i = 0; i < 4; i++) {
		float dis_t = hitSphere(sphere[i], r);
		if (dis_t > 0 && dis_t < dis) {
			dis = dis_t;
			hitSphereIndex = i;
			ifHitSphere = true;
		}
	}*/

	if (ifHitSphere) {
		rec.Pos = r.origin + r.hitMin * r.direction;
		rec.Normal = normalize(rec.Pos - sphere[hitSphereIndex].center);
		rec.material.baseColor = sphere[hitSphereIndex].albedo;
		rec.material.transmission = sphere[hitSphereIndex].materialIndex;
		return true;
	}
	if (ifHitTriangleFloor) {
		// rec.Pos = r.origin + r.hitMin * r.direction;
		// rec.Normal = getTriangleNormal(triFloor[hitTriangleIndex]);
		// // rec.albedo = vec3(0.87, 0.87, 0.87); 
		// rec.material.baseColor = vec3(0.8, 0.8, 0.8);  // 改为淡青色玻璃色调
		// rec.material.transmission = 1;
		// return true;
	}
	else if (ifHitTriangleMesh) {
		return true;
	}
	else return false;
}

vec3 random_in_unit_sphere() {
	vec3 p;
	do {
		p = 2.0 * vec3(rand(), rand() ,rand()) - vec3(1, 1, 1);
	} while (dot(p, p) >= 1.0);
	return p;
}

vec3 diffuseReflection(vec3 Normal) {
	return normalize(Normal + random_in_unit_sphere());
}

vec3 metalReflectionRough(vec3 rayIn, vec3 Normal, float roughness) {
	vec3 jitter = roughness * random_in_unit_sphere();
	return normalize(rayIn - 2 * dot(rayIn, Normal) * Normal + jitter);
}

vec3 metalReflection(vec3 rayIn, vec3 Normal) {
	vec3 jitter = 0.01 * random_in_unit_sphere();
	return normalize(rayIn - 2 * dot(rayIn, Normal) * Normal + jitter);
}

// =========================================================
// vec3 shading(Ray r) {
// 	vec3 color = vec3(0.0,0.0,0.0);
	
// 	vec3 throughput = vec3(1.0);
//     const float RR_THRESHOLD = 0.8; // 俄罗斯轮盘赌阈值

// 	for (int depth = 0; depth < 50; depth++) {
// 		// 俄罗斯轮盘赌终止条件
//         if (depth > 3) {
//             float q = max(throughput.r, max(throughput.g, throughput.b));
//             if (rand() > q * RR_THRESHOLD) break;
//             throughput /= q * RR_THRESHOLD;
//         }

// 		if (!hitWorld(r)) {

// 			float t = 0.5*(r.direction.y + 1.0);
// 			// 变背景代码
// 			vec3 background = (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
// 			// color *= background;
// 			color += background;
// 			// 改为纯白色背景：
// 			// color *= vec3(1.0); // RGB全为1.0即纯白
// 			break;
// 		}

		
// 		if(rec.materialIndex == 0)
// 		{
// 			r.direction = diffuseReflection(rec.Normal);
// 			throughput *= rec.albedo;
// 		}
// 		else if(rec.materialIndex == 1)
// 		{
// 			r.direction = metalReflection(r.direction, rec.Normal);
// 			throughput *= rec.albedo;
// 		}
// 		else if(rec.materialIndex == 2)
// 		{
// 			float cosTheta = dot(-r.direction, rec.Normal);
// 			color += throughput * rec.emissive * max(cosTheta, 0.0); // 正确累加直接光照
// 			break; // 物理正确的路径终止
// 		}

// 		r.origin = rec.Pos;
// 		r.hitMin = 100000;
		
// 	}
// 	color = color * throughput;
// 	return color;
// }
// =========================================================

// vec3 shading(Ray r) {
// 	vec3 accumulatedColor = vec3(0.0, 0.0, 0.0); // 初始化累积颜色为黑色
// 	vec3 throughput = vec3(1.0, 1.0, 1.0);     // 初始化光线能量为白色

// 	const int MAX_DEPTH = 50; // 限制最大弹射次数，防止无限循环

// 	for (int i = 0; i < MAX_DEPTH; i++) {
// 		if (hitWorld(r)) {
			
// 			switch(rec.material.transmission) {
// 				case -1: // Emissive Material
// 					// 如果击中光源，将光源颜色乘以当前能量，加到累积颜色，然后终止路径
// 					accumulatedColor += throughput * rec.material.emissive;
// 					return accumulatedColor; // 或者使用 break; 如果希望继续追踪（虽然对于简单光源通常是终止）
// 					// break; 
// 				case 0: // Diffuse Material
// 					throughput *= rec.material.baseColor; // 能量根据反射率衰减
// 					r.direction = diffuseReflection(rec.Normal); // 计算新的反射方向
// 					break;
// 				case 1: // Metallic Material
// 					throughput *= rec.material.baseColor; // 能量根据反射率衰减
// 					r.direction = metalReflectionRough(r.direction, rec.Normal, rec.material.roughness); // 计算新的反射方向
// 					break;
// 				// 可以添加更多材质类型，例如折射等
// 			}

// 			// 俄罗斯轮盘赌 (可选，用于优化性能)
// 			// if (i > 3) {
// 			// 	 float p = max(throughput.x, max(throughput.y, throughput.z));
// 			// 	 if (rand() > p) {
// 			// 		 break; // 随机终止路径
// 			// 	 }
// 			// 	 throughput /= p; // 补偿能量
// 			// }


// 			r.origin = rec.Pos; // 更新光线起点为碰撞点
// 			r.hitMin = 100000.0; // 重置下次碰撞的最大距离
// 		}
// 		else {
// 			// Ray Missed - Add background/environment light
// 			float t = 0.5*(r.direction.y + 1.0);
// 			vec3 backgroundColor = (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
// 			// vec3 backgroundColor = vec3(0.0); // 或者黑色背景

// 			accumulatedColor += throughput * backgroundColor; // 将背景光贡献加到累积颜色
// 			break; // 光线射出场景，终止循环
// 		}
// 	}
// 	// 如果循环因为达到 MAX_DEPTH 而结束，也返回当前累积的颜色
// 	// （可能路径没有完全终止于光源或背景）
// 	return accumulatedColor; 
// }

// =========================================================
vec3 shading(Ray r) {
	vec3 color = vec3(1.0,1.0,1.0);
	int flag = 1;
	for (int i = 0; i < 200; i++) {
		if (hitWorld(r)) {
			if (flag == 0) break;
			switch(rec.material.transmission) {
				case -1:
					color *= rec.material.emissive * 0.3;
					flag = 0;
					break;
				case 0:
					r.direction = diffuseReflection(rec.Normal);
					color *= rec.material.baseColor;
					break;
				case 1:
					r.direction = metalReflectionRough(r.direction, rec.Normal, rec.material.roughness);
					color *= rec.material.baseColor;
					break;
			}

			r.origin = rec.Pos;
			r.hitMin = 100000;
		}
		else {

			// if (i == 1) {
			// 	vec3 lightPos = vec3(-4.0, 4.0, 4.0);
			// 	vec3 lightDir = normalize(lightPos - rec.Pos);
			// 	float diff = 0.5 * max(dot(rec.Normal, lightDir), 0.0) + 0.5;
			// 	color *= vec3(diff, diff, diff);
			// }
			// else {
				// float t = 0.5*(r.direction.y + 1.0);
				// // 变背景代码
				// color *= (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
				// 改为纯白色背景：
				color *= vec3(1.0); // RGB全为1.0即纯白
			// }
			break;
		}
	}
	return color;
}
// =========================================================

// =========================================================
// // 实现新的shade函数，使用参考代码中的物理模型
// vec3 shade(HitResult hit_obj, vec3 wo) {
//     vec3 Lo = vec3(1.0);
//     int flag = 1;
    
//     for (int i = 0; i < 5; i++) { // 将迭代次数限制在合理范围内
//         if (flag == 0) break;
        
//         switch(hit_obj.material.transmission) {
//             case -1: // 发光材质
//                 Lo *= hit_obj.material.emissive;
//                 flag = 0;
//                 break;
//             case 0: // 漫反射
//                 {
//                     // 采样下一个方向
//                     vec3 dir_next = diffuseReflection(hit_obj.normal);
                    
//                     // 创建下一条光线
//                     Ray ray;
//                     ray.origin = hit_obj.hitPoint;
//                     ray.direction = dir_next;
//                     ray.hitMin = 100000.0;
                    
//                     // 计算下一次碰撞
//                     HitResult nextHit = hitWorld(ray);
                    
//                     if (nextHit.isHit) {
//                         // 衰减颜色
//                         Lo *= hit_obj.material.baseColor;
                        
//                         // 继续追踪
//                         hit_obj = nextHit;
//                         wo = -dir_next;
//                     } else {
//                         // 击中环境/背景
//                         float t = 0.5 * (dir_next.y + 1.0);
//                         Lo *= hit_obj.material.baseColor * ((1.0 - t) * vec3(1.0) + t * vec3(0.5, 0.7, 1.0));
//                         flag = 0;
//                     }
//                 }
//                 break;
//             case 1: // 金属材质
//                 {
//                     // 采样下一个方向
//                     vec3 dir_next = metalReflectionRough(wo, hit_obj.normal, hit_obj.material.roughness);
                    
//                     // 创建下一条光线
//                     Ray ray;
//                     ray.origin = hit_obj.hitPoint;
//                     ray.direction = dir_next;
//                     ray.hitMin = 100000.0;
                    
//                     // 计算下一次碰撞
//                     HitResult nextHit = hitWorld(ray);
                    
//                     if (nextHit.isHit) {
//                         // 衰减颜色
//                         Lo *= hit_obj.material.baseColor;
                        
//                         // 继续追踪
//                         hit_obj = nextHit;
//                         wo = -dir_next;
//                     } else {
//                         // 击中环境/背景
//                         float t = 0.5 * (dir_next.y + 1.0);
//                         Lo *= hit_obj.material.baseColor * ((1.0 - t) * vec3(1.0) + t * vec3(0.5, 0.7, 1.0));
//                         flag = 0;
//                     }
//                 }
//                 break;
//         }
//     }
    
//     return Lo;
// }

// =========================================================

vec3 getBoundp(Bound3f bound, int i) {
	return (i == 0) ? bound.pMin : bound.pMax;
}
bool IntersectBound(Bound3f bounds, Ray ray, vec3 invDir, bool dirIsNeg[3]) {
	// Check for ray intersection against $x$ and $y$ slabs
	float tMin = (getBoundp(bounds, int(dirIsNeg[0])).x - ray.origin.x) * invDir.x;
	float tMax = (getBoundp(bounds, 1 - int(dirIsNeg[0])).x - ray.origin.x) * invDir.x;
	float tyMin = (getBoundp(bounds, int(dirIsNeg[1])).y - ray.origin.y) * invDir.y;
	float tyMax = (getBoundp(bounds, 1 - int(dirIsNeg[1])).y - ray.origin.y) * invDir.y;

	// Update _tMax_ and _tyMax_ to ensure robust bounds intersection
	if (tMin > tyMax || tyMin > tMax) return false;
	if (tyMin > tMin) tMin = tyMin;
	if (tyMax < tMax) tMax = tyMax;

	// Check for ray intersection against $z$ slab
	float tzMin = (getBoundp(bounds, int(dirIsNeg[2])).z - ray.origin.z) * invDir.z;
	float tzMax = (getBoundp(bounds, 1 - int(dirIsNeg[2])).z - ray.origin.z) * invDir.z;

	// Update _tzMax_ to ensure robust bounds intersection
	if (tMin > tzMax || tzMin > tMax) return false;
	if (tzMin > tMin) tMin = tzMin;
	if (tzMax < tMax) tMax = tzMax;

	return tMax > 0;
}









