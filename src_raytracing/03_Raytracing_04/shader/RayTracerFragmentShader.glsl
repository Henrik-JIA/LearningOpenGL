#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

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

struct Triangle {
	vec3 v0, v1, v2;
	vec3 n0, n1, n2;
	vec2 u0, u1, u2;
};
uniform Triangle triFloor[2];

uniform sampler2D texMesh;
uniform int meshNum;
uniform sampler2D texBvhNode;
uniform int bvhNodeNum;

struct hitRecord {
	vec3 Normal;
	vec3 Pos;
	vec3 albedo;
	int materialIndex;
	float rayHitMin;
};
hitRecord rec;


float At(sampler2D dataTex, float index);
// 返回值：ray到球交点的距离
float hitSphere(Sphere s, Ray r);
float hitTriangle(Triangle tri, Ray r);
bool hitWorld(Ray r);
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

	vec3 curColor = shading(cameraRay);
	
	curColor = (1.0 / float(camera.LoopNum))*curColor + (float(camera.LoopNum - 1) / float(camera.LoopNum))*hist;
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
		rec.albedo = vec3(0.83, 0.73, 0.1); 
		// rec.albedo = rec.Normal * 0.5 + 0.5; // 法线可视化
		rec.rayHitMin = ray.hitMin;
		rec.materialIndex = 1;
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
	int offset = index * (9 + 9 + 6);
	tri_t.v0.x = At(texMesh, float(offset));
	tri_t.v0.y = At(texMesh, float(offset + 1));
	tri_t.v0.z = At(texMesh, float(offset + 2));

	tri_t.v1.x = At(texMesh, float(offset + 3));
	tri_t.v1.y = At(texMesh, float(offset + 4));
	tri_t.v1.z = At(texMesh, float(offset + 5));

	tri_t.v2.x = At(texMesh, float(offset + 6));
	tri_t.v2.y = At(texMesh, float(offset + 7));
	tri_t.v2.z = At(texMesh, float(offset + 8));
	return tri_t;
}

vec3 getTriangleNormal(Triangle tri) {
	return normalize(cross(tri.v2 - tri.v0, tri.v1 - tri.v0));
}

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
		rec.albedo = sphere[hitSphereIndex].albedo;
		rec.materialIndex = sphere[hitSphereIndex].materialIndex;
		return true;
	}
	if (ifHitTriangleFloor) {
		rec.Pos = r.origin + r.hitMin * r.direction;
		rec.Normal = getTriangleNormal(triFloor[hitTriangleIndex]);
		// rec.albedo = vec3(0.87, 0.87, 0.87); 
		rec.albedo = vec3(0.8, 0.8, 0.8);  // 改为淡青色玻璃色调
		rec.materialIndex = 1;
		return true;
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

vec3 metalReflection(vec3 rayIn, vec3 Normal) {
	return normalize(rayIn - 2 * dot(rayIn, Normal) * Normal + 0.01 * random_in_unit_sphere());
}

vec3 shading(Ray r) {
	vec3 color = vec3(1.0,1.0,1.0);
	for (int i = 0; i < 100; i++) {
		if (hitWorld(r)) {
			
			if(rec.materialIndex == 0)
				r.direction = diffuseReflection(rec.Normal);
			else if(rec.materialIndex == 1)
				r.direction = metalReflection(r.direction, rec.Normal);

			r.origin = rec.Pos;
			r.hitMin = 100000;

			color *= rec.albedo;
		}
		else {

			if (i == 1) {
				vec3 lightPos = vec3(-4.0, 4.0, 4.0);
				vec3 lightDir = normalize(lightPos - rec.Pos);
				float diff = 0.5 * max(dot(rec.Normal, lightDir), 0.0) + 0.5;
				color *= vec3(diff, diff, diff);
			}
			else {
				float t = 0.5*(r.direction.y + 1.0);
				// 变背景代码
				color *= (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
				// 改为纯白色背景：
				// color *= vec3(1.0); // RGB全为1.0即纯白
			}
			break;
		}
	}
	return color;
}

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









