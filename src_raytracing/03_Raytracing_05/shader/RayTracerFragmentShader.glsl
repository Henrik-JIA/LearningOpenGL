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
    vec3 pMin;         // 包围盒最小顶点坐标（左下前）
    vec3 pMax;         // 包围盒最大顶点坐标（右上后）
    int nPrimitives;   // 当前节点包含的图元（三角形）数量
    int axis;          // 空间分割轴（0:X轴，1:Y轴，2:Z轴）
    int childOffset;   // 子节点偏移量（内部节点）或图元起始索引（叶子节点）
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
	vec3 p0, p1, p2;
	vec3 n0, n1, n2;
	vec2 u0, u1, u2;
	Material material;
};

uniform sampler2D texMesh;
uniform int meshNum;
uniform sampler2D texBvhNode;
uniform int bvhNodeNum;

struct hitRecord {
	bool isHit;
	bool isInside;
	float rayHitMin;
	vec3 Pos;
	vec3 Normal;
	vec3 viewDir;
	Material material; 
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
Triangle getTriangle(int index);
bool IntersectBound(Bound3f bounds, Ray ray, vec3 invDir, bool dirIsNeg[3]);


// 在Camera结构体后添加以下声明
bool IntersectBVH(Ray ray);
vec3 shade(hitRecord hit_obj, vec3 wo);


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

	vec3 curColor = vec3(0.0, 0.0, 0.0);
	int N = 2;
	for (int i = 0; i < N; i++) 
	{
		if(IntersectBVH(cameraRay)) {
			curColor += shading(cameraRay);
		}else{
			float t = 0.5 * (cameraRay.direction.y + 1.0);
			vec3 bgColor = (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
			// vec3 bgColor = vec3(0.0, 0.0, 0.0);
			curColor += bgColor;
		}
	}
	curColor /= float(N);
	


	// vec3 curColor = shading(cameraRay);
	
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

// 在单位球面上随机生成一个点
vec3 random_in_unit_sphere() {
	vec3 p;
	do {
		p = 2.0 * vec3(rand(), rand() ,rand()) - vec3(1, 1, 1);
	} while (dot(p, p) >= 1.0);
	return p;
}

// 在单位半球面上随机生成一个点
vec3 random_in_unit_hemisphere(vec3 Normal) {
    vec3 p;
    do {
        p = 2.0 * vec3(rand(), rand(), rand()) - vec3(1.0);
        p = normalize(p); // 单位球面
    } while(dot(p, Normal) < 0.0); // 确保在法线半球
    return p;
}

// ********* BVH加速 ********* // 

// ==========================================================
// 经典的射线与AABB相交检测算法（Slab Method）
vec3 getBoundp(Bound3f bound, int i) {
	return (i == 0) ? bound.pMin : bound.pMax;
}

// 射线与AABB相交检测算法
bool IntersectBound(Bound3f bounds, Ray ray, vec3 invDir, bool dirIsNeg[3]) {
	// Check for ray intersection against $x$ and $y$ slabs
	// 检查射线与AABB的x和y面相交
	float tMin = (getBoundp(bounds, int(dirIsNeg[0])).x - ray.origin.x) * invDir.x;
	float tMax = (getBoundp(bounds, 1 - int(dirIsNeg[0])).x - ray.origin.x) * invDir.x;
	float tyMin = (getBoundp(bounds, int(dirIsNeg[1])).y - ray.origin.y) * invDir.y;
	float tyMax = (getBoundp(bounds, 1 - int(dirIsNeg[1])).y - ray.origin.y) * invDir.y;

	// Update _tMax_ and _tyMax_ to ensure robust bounds intersection
	// 更新tMax和tyMax以确保鲁棒的AABB相交
	if (tMin > tyMax || tyMin > tMax) return false;
	if (tyMin > tMin) tMin = tyMin;
	if (tyMax < tMax) tMax = tyMax;

	// Check for ray intersection against $z$ slab
	// 检查射线与AABB的z面相交
	float tzMin = (getBoundp(bounds, int(dirIsNeg[2])).z - ray.origin.z) * invDir.z;
	float tzMax = (getBoundp(bounds, 1 - int(dirIsNeg[2])).z - ray.origin.z) * invDir.z;

	// Update _tzMax_ to ensure robust bounds intersection
	// 更新tzMax以确保鲁棒的AABB相交
	if (tMin > tzMax || tzMin > tMax) return false;
	if (tzMin > tMin) tMin = tzMin;
	if (tzMax < tMax) tMax = tzMax;

	return tMax > 0;
}
// ==========================================================

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
	tri_t.p0.x = At(texMesh, float(offset));
	tri_t.p0.y = At(texMesh, float(offset + 1));
	tri_t.p0.z = At(texMesh, float(offset + 2));
	tri_t.p1.x = At(texMesh, float(offset + 3));
	tri_t.p1.y = At(texMesh, float(offset + 4));
	tri_t.p1.z = At(texMesh, float(offset + 5));
	tri_t.p2.x = At(texMesh, float(offset + 6));
	tri_t.p2.y = At(texMesh, float(offset + 7));
	tri_t.p2.z = At(texMesh, float(offset + 8));

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

// 用于计算光线与三角形相交的函数，采用两步法实现，非Möller-Trumbore算法
// 返回值：ray到三角形交点的距离
float hitTriangle(Triangle tri, Ray r) {
	// 1. 找到三角形所在平面法向量
	vec3 A = tri.p1 - tri.p0;
	vec3 B = tri.p2 - tri.p0;
	vec3 N = normalize(cross(A, B));

	// 2. Ray与平面平行，没有交点
	if (dot(N, r.direction) == 0) return -1.0;

	// 3. 计算光线与平面交点参数t
	float D = -dot(N, tri.p0);
	float t = -(dot(N, r.origin) + D) / dot(N, r.direction);
	if (t < 0) return -1.0; // 排除背面相交
	
	// 4. 使用重心坐标测试点是否在三角形内
	// 交点坐标用射线方程来表示
	vec3 pHit = r.origin + t * r.direction;
	// 判断交点是否在三角形内，实使用了向量叉乘，用于判断光线与三角形的交点是否位于三角形内部。这是通过三次边缘检测实现的。
	vec3 edge0 = tri.p1 - tri.p0;// 边向量
	vec3 C0 = pHit - tri.p0; // 交点到起点的向量
	if (dot(N, cross(edge0, C0)) < 0) return -1.0; // 叉乘结果与法向量点积小于0，说明交点在三角形外
	vec3 edge1 = tri.p2 - tri.p1;
	vec3 C1 = pHit - tri.p1;
	if (dot(N, cross(edge1, C1)) < 0) return -1.0;
	vec3 edge2 = tri.p0 - tri.p2;
	vec3 C2 = pHit - tri.p2;
	if (dot(N, cross(edge2, C2)) < 0) return -1.0;

	// 光线与三角形相交
	// 5. 返回有效相交距离（减去ε避免自相交）
	return t - 0.000001;
}

vec3 getTriangleNormal(Triangle tri) {
	return normalize(cross(tri.p2 - tri.p0, tri.p1 - tri.p0));
}

LinearBVHNode getLinearBVHNode(int nodeIndex) {
	const int FLOATS_PER_NODE = 9; // 每个节点占用的float数
    int storageOffset = nodeIndex * FLOATS_PER_NODE;

	LinearBVHNode node;

	// 解析包围盒最小点（3个float）
	node.pMin = vec3(
			At(texBvhNode, float(storageOffset + 0)), 
			At(texBvhNode, float(storageOffset + 1)), 
			At(texBvhNode, float(storageOffset + 2))
		);

	// 解析包围盒最大点（3个float）
	node.pMax = vec3(
			At(texBvhNode, float(storageOffset + 3)), 
			At(texBvhNode, float(storageOffset + 4)), 
			At(texBvhNode, float(storageOffset + 5))
		);
	
	// 解析叶节点中三角形数量（1个int）
	node.nPrimitives = int(At(texBvhNode, float(storageOffset + 6)));

	// 解析分割轴（0:X,1:Y,2:Z）
	node.axis = int(At(texBvhNode, float(storageOffset + 7)));
	
	// 解析子节点偏移量（1个int）
	node.childOffset = int(At(texBvhNode, float(storageOffset + 8)));

	// 返回BVH节点
    return node;
}

bool IntersectBVH(Ray ray) {
	// if (!bvhTree.nodes) return false;
	bool hit = false;
	rec.isHit = false;
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
		vec3 rawNormal = getTriangleNormal(tri);
		rec.isHit = true;
		rec.isInside = (dot(rawNormal, ray.direction) > 0.0);
		rec.rayHitMin = ray.hitMin;
		rec.Pos = ray.origin + ray.hitMin * ray.direction;
		// 我也不清楚模型的顶点坐标是顺时针还是逆时针，加负号效果是对的。
		// rec.Normal = -getTriangleNormal(tri);
		// 下面注释的glsl代码可以解决面法向量始终超外，但会影响帧率
		rec.Normal = (dot(rawNormal, -ray.direction) > 0.0) ? rawNormal : -rawNormal;
		rec.viewDir = -ray.direction;
		// rec.albedo = vec3(0.83, 0.73, 0.1); 
		// rec.albedo = rec.Normal * 0.5 + 0.5; // 法线可视化
		rec.material = tri.material; 
		// rec.materialIndex = 0;
		// rec.materialIndex = tri.materialType;
	}
	return hit;
}

// =========================================================
// 将切线空间向量转换到世界空间
vec3 toWorld(vec3 v, vec3 N) {
	vec3 helper = vec3(1, 0, 0);
	if(abs(N.x)>0.999) helper = vec3(0, 0, 1);
	vec3 tangent = normalize(cross(N, helper));
	vec3 bitangent = normalize(cross(N, tangent));
	return v.x * tangent + v.y * bitangent + v.z * N;
}

// 采样函数
vec3 sample_(vec3 wo, vec3 N, Material material){
	vec3 dir;
	switch(material.transmission) {
		case 0: {
			// 余弦加权半球采样
			float r0 = rand();
			float r1 = rand();
			float phi = 2.0 * PI * r0;
			float z = sqrt(1.0 - r1);  // 余弦加权分布
			float r = sqrt(r1);
			vec3 localRay = vec3(r * cos(phi), r * sin(phi), z);
			
			// float z = rand();
			// float r = max(0, sqrt(1.0 - z*z));
			// float phi = 2.0 * PI * rand();
			// vec3 localRay = vec3(r * cos(phi), r * sin(phi), z);

			// 转换到场景中的坐标系
			dir = toWorld(localRay, N);
			break;
		}
		case 1: {
			// 随机一个 ε 和 φ
			float r0 = rand();
			float r1 = rand();
			float a2 = material.roughness * material.roughness;
			float phi = 2 * PI * r1;
			float theta = cos(sqrt((1 - r0) / (r0 * (a2 - 1) + 1)));
			// 单位向量半径就直接 1 了，转换为直角坐标系只需要用到 r*sinθ，所以这里直接乘上去了
			float r = sin(theta);
			dir = reflect(-wo, toWorld(vec3(r * cos(phi), r * sin(phi), cos(theta)), N));
			break;
		}
	}
	return dir;
}

// =========================================================
// 计算GGX分布
float DistributionGGX(vec3 N, vec3 H, float a){
	float a2     = a*a;
	float NdotH  = max(dot(N, H), 0.f);
	float NdotH2 = NdotH*NdotH;
	float nom    = a2;
	float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
	denom        = PI * denom * denom;

	return nom / denom;
}

// 计算概率密度函数
float pdf_(vec3 wo, vec3 wi, vec3 N, Material material){
	float pdf;
	float cosalpha_i_N = dot(wi, N);
	switch(material.transmission) {
		case 0:{
			if (cosalpha_i_N > EPSILON)
				pdf =  0.5 / PI;
			else
				pdf =  0.0f;
			break;
		}
		case 1: {
			if (cosalpha_i_N > EPSILON) {
				vec3 h = normalize(wo + wi);
				pdf = DistributionGGX(N, h, material.roughness) * dot(N, h) / (4.f * dot(wo, h));
			}else{
				pdf =  0.0f;
			}
			break;
		}
	}
	return pdf;
}

// =========================================================
// Schlick-GGX几何衰减项近似
// 参数：
//   NdotV - 法线与视线方向的点积
//   k     - 根据粗糙度调整的参数（直接光照k=(α+1)^2/8，间接光照k=α^2/2）
// 返回值：几何衰减因子
float GeometrySchlickGGX(float NdotV, float k)
{
	float nom   = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}

// Smith方法计算联合几何衰减
// 参数：
//   NdotV - 法线与视线方向的点积
//   NdotL - 法线与光线方向的点积  
//   k     - 几何衰减参数
// 返回值：综合几何衰减因子（观察方向与光线方向的几何函数乘积）
float GeometrySmith(float NdotV, float NdotL, float k)
{
	float ggx1 = GeometrySchlickGGX(NdotV, k); // 视线方向衰减
	float ggx2 = GeometrySchlickGGX(NdotL, k); // 光线方向衰减

	return ggx1 * ggx2; // 联合衰减
}

// 线性插值函数（GLSL已有内置mix函数，此实现用于演示）
// 参数：
//   a - 起始值
//   b - 结束值
//   t - 插值系数[0,1]
// 返回值：a和b之间的线性插值结果
vec3 lerp(vec3 a, vec3 b, float t){
	return a * (1 - t) + b * t;
}

// Schlick菲涅尔近似
// 参数：
//   cosTheta - 入射方向与表面法线的夹角余弦值
//   F0       - 基础反射率（垂直入射时的反射率）
// 返回值：菲涅尔反射率（不同角度的反射强度）
vec3 fresnelSchlick(float cosTheta, vec3 F0){
	return F0 + (vec3(1.0) - F0) * pow(1.0 - cosTheta, 5.0);
}

// 计算BRDF
// 参数：
//   wi - 入射方向
//   wo - 出射方向
//   N  - 法线
//   material - 材质
// 返回值：BRDF值
vec3 eval_(vec3 wi, vec3 wo, vec3 N, Material material) {
	vec3 f_r;
	switch(material.transmission) {
		case 0:{
			float cosalpha = dot(N, wo);
			if(cosalpha > EPSILON) {
				f_r = material.baseColor / PI;
			}else {
				f_r = vec3(0.0f);
			}
			break;
		}
		case 1: {
			// cosθ是入射光和法线的夹角，也就是光源方向和法线方向的夹角
			float cosTheta = dot(N, wo);
			if(cosTheta > EPSILON) {
				vec3 V = wi;
				vec3 L = wo;
				vec3 H = normalize(V + L);
				float NdotV = max(dot(N, V), EPSILON);
				float NdotL = cosTheta;
				// 直接光照情况下的 k 公式
				float k = (material.roughness + 1.f) * (material.roughness + 1.f) / 8.f;
				float D = DistributionGGX(N, H, material.roughness);
				float G = GeometrySmith(NdotV, NdotL, k);

				vec3 F0 = vec3(0.04f);
				F0 = lerp(F0, material.baseColor, material.metallic);
				vec3 F = fresnelSchlick(dot(H, V), F0);
				// float F;
				// fresnel(-V, N, ior, F);
				vec3 fs = D * G * F / (4.f * NdotV  * NdotL);

				// std::cout << fs <<std::endl;
				// 菲涅尔项就是 ks， kd = 1-ks;
				vec3 fr =  material.baseColor / PI;

				// return (Vector3f(1.0f) - F0) * fr + F0 * fs;
				f_r = (vec3(1.0f) - F) * (1 - material.metallic) * fr + fs;
			}else {
				f_r = vec3(0.f);
			}
			break;
		}
	}
	return f_r;
}

// =========================================================

vec3 diffuseReflection(vec3 Normal) {
	// return normalize(Normal + random_in_unit_sphere());
	return normalize(Normal + random_in_unit_hemisphere(Normal));
}

vec3 metalReflectionRough(vec3 rayIn, vec3 Normal, float roughness) {
	vec3 jitter = roughness * random_in_unit_sphere();
	return normalize(rayIn - 2 * dot(rayIn, Normal) * Normal + jitter);
}

vec3 metalReflection(vec3 rayIn, vec3 Normal) {
	vec3 jitter = 0.01 * random_in_unit_sphere();
	return normalize(rayIn - 2 * dot(rayIn, Normal) * Normal + jitter);
}

// ========================================================

// vec3 shading(Ray r) {
// 	vec3 throughput = vec3(1.0); // 辐射通量
// 	vec3 Lo = vec3(0.0, 0.0, 0.0); // 出射辐射率
// 	float P_RR = 0.8; // 俄罗斯轮盘赌概率

// 	for(int depth=0; depth<20; depth++) { 
// 		if (rand() > P_RR) break;  // 俄罗斯轮盘赌判断

// 		if (!IntersectBVH(r)) break; // 光线没有与场景中的物体相交，则终止

// 		switch(rec.material.transmission) {
// 			case -1: // 光源
// 			{
// 				Lo += throughput * rec.material.emissive / P_RR;
//                 return Lo;  // 击中光源立即返回
// 			}
// 			case 0: // 漫反射
// 			{
// 				vec3 wo = rec.viewDir;
// 				vec3 wi = sample_(wo, rec.Normal, rec.material);
// 				vec3 f_r = rec.material.baseColor; // 漫反射系数
// 				float cosine = max(0.0, dot(rec.Normal, wi)); // 余弦值
// 				float pdf = pdf_(wo, wi, rec.Normal, rec.material); // 概率密度函数
// 				throughput *= (f_r * cosine / pdf) / P_RR;

// 				// 更新光线
//                 r.origin = rec.Pos;
//                 r.direction = wi;
//                 r.hitMin = 100000;
// 				break;
// 			}
// 		}
// 	}
// 	return Lo;
// }

vec3 shading(Ray r) {
    vec3 throughput = vec3(1.0);
    vec3 Lo = vec3(0.0);
	int flag = 1;
    // float P_RR = 0.8;
	float P_RR = 1.0;

    for(int depth=0; depth<60; depth++) {
		if (flag == 0) break;

		// 俄罗斯轮盘赌判断
        if (depth > 3 && rand() > P_RR) break;

        // 根据命中点材质类型处理光照
        switch(rec.material.transmission) {
            case -1: // 光源材质
            {   
				// float cosine = max(0.0, dot(rec.Normal, rec.viewDir));
				// Lo = throughput * (rec.material.emissive * cosine);
				Lo = throughput * (rec.material.emissive * 0.2);
				flag = 0;
                break;
			}
            case 0: // 漫反射材质
            {
				vec3 dir_next = sample_(rec.viewDir, rec.Normal, rec.material);
				float pdf = pdf_(rec.viewDir, dir_next, rec.Normal, rec.material);
				// vec3 f_r = rec.material.baseColor / PI; // 漫反射系数
				vec3 f_r = eval_(dir_next, rec.viewDir, rec.Normal, rec.material);
				float cosine = max(0.0, dot(dir_next, rec.Normal));
				if (pdf > EPSILON) 
				{
					Ray rayNext;
					rayNext.origin = rec.Pos + rec.Normal * 0.001;
					rayNext.direction = dir_next;
					rayNext.hitMin = 100000;
					bool hitNext = IntersectBVH(rayNext);
					if (hitNext) 
					{
						throughput = throughput * (f_r * cosine) / pdf / P_RR;
					}
					else
					{
						throughput = vec3(0.0);
						flag = 0;
						break;
					}
				}
				else
				{
					throughput = vec3(0.0);
					flag = 0;
					break;
				}
                break;
            }
			case 1: // 金属
			{
				vec3 dir_next = sample_(rec.viewDir, rec.Normal, rec.material);
				float pdf = pdf_(rec.viewDir, dir_next, rec.Normal, rec.material);
				// vec3 f_r = rec.material.baseColor / PI; // 漫反射系数
				vec3 f_r = eval_(dir_next, rec.viewDir, rec.Normal, rec.material);
				float cosine = max(0.0, dot(dir_next, rec.Normal));
				if (pdf > EPSILON) 
				{
					Ray rayNext;
					rayNext.origin = rec.Pos + rec.Normal * 0.001;
					rayNext.direction = dir_next;
					rayNext.hitMin = 100000;
					bool hitNext = IntersectBVH(rayNext);
					if (hitNext) 
					{
						throughput = throughput * (f_r * cosine) / pdf / P_RR;
					}
					else
					{
						throughput = vec3(0.0);
						flag = 0;
						break;
					}
				}
				else
				{
					throughput = vec3(0.0);
					flag = 0;
					break;
				}
                break;
			}
        }
    }
    return Lo;
}
// =========================================================

// vec3 shading(Ray r) {
	
// 	vec3 Lo = vec3(0.0, 0.0, 0.0);

// 	float P_RR = 0.8; // 俄罗斯轮盘赌概率
// 	if (rand() > P_RR) return vec3(0.0);

// 	// 判断入射光线是否与场景中的物体相交
// 	if (IntersectBVH(r)) 
// 	{
// 		switch(rec.material.transmission) 
// 		{
// 			case -1: // 光源
// 			{
// 				vec3 wo = rec.viewDir;
// 				vec3 wi = sample_(wo, rec.Normal, rec.material);
// 				vec3 f_r = rec.material.baseColor; // 漫反射系数
// 				float cosine = max(0.0, dot(rec.Normal, wi)); // 余弦值
// 				float pdf = pdf_(wo, wi, rec.Normal, rec.material); // 概率密度函数
// 				vec3 L_i = rec.material.emissive; // 光源辐射率
// 				Lo = (L_i * f_r * cosine / pdf)	/ P_RR; // 计算光照
// 				break;
// 			}
// 			case 0: // 漫反射
// 			{
// 				vec3 wo = rec.viewDir;
// 				vec3 wi = sample_(wo, rec.Normal, rec.material);
// 				r.direction = wi;
// 				r.origin = rec.Pos;
// 				r.hitMin = 100000;
// 				vec3 f_r = rec.material.baseColor; // 漫反射系数
// 				float cosine = max(0.0, dot(rec.Normal, wi)); // 余弦值
// 				float pdf = pdf_(wo, wi, rec.Normal, rec.material); // 概率密度函数
// 				Lo = (shading(r) * f_r * cosine / pdf) / P_RR;
// 				break;
// 			}
// 			case 1: // 金属
// 			{
// 				break;
// 			}
				

// 		}
// 	}

// 	return Lo;
// }


// =========================================================


// =========================================================
// vec3 shading(Ray r) {
// 	vec3 Lo = vec3(1.0,1.0,1.0); // 出射辐射率
// 	vec3 rayNext = vec3(0.0,0.0,0.0); // 下一条光线方向
// 	vec3 f_r = vec3(0.0,0.0,0.0);
// 	float cosine = 0.0;
// 	float pdf = 0.0;
// 	int flag = 1; // 标志位，用于控制光线追踪遇到自发光材质（光源）时的提前终止
// 	int max_depth = 10; // 最大深度
// 	for (int i = 0; i < max_depth; i++) {
// 		if (IntersectBVH(r)) {
// 			if (flag == 0) break;
// 			switch(rec.material.transmission) {
// 				case -1: // 光源
// 					Lo *= rec.material.emissive;
// 					flag = 0;
// 					break;
// 				case 0: // 漫反射
// 					rayNext = diffuseReflection(rec.Normal);
// 					f_r = rec.material.baseColor / PI;
// 					// pdf = 0.5 / PI;
// 					cosine = max(0.0, dot(rec.Normal, rayNext));
// 					Lo *= (f_r * cosine);
// 					break;
// 				case 1: // 金属
// 					rayNext = metalReflectionRough(r.direction, rec.Normal, rec.material.roughness);
// 					Lo *= rec.material.baseColor;
//                 break;
//             }
// 			r.direction = rayNext;
// 			r.origin = rec.Pos;
// 			r.hitMin = 100000;
// 		}
// 	}
// 	return Lo;
// }
// =========================================================

// =========================================================







