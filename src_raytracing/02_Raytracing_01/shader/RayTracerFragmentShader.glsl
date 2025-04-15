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
};

uniform float randOrigin;
uint wseed;
float rand(void);

// 球体数据结构
struct Sphere {
    vec3 center;    // 球心坐标 (x, y, z)
    float radius;   // 球体半径
    vec3 albedo;    // 表面基础颜色（反射率）
    int materialIndex; // 材质类型索引（0:漫反射 1:金属 2:电介质...）
};
uniform Sphere sphere[4];

// 三角形数据结构
struct Triangle {
    vec3 v0, v1, v2; // 三个vec3(x, y, z)顶点坐标
    vec3 n0, n1, n2; // 对应顶点的法线向量
    vec2 u0, u1, u2; // 纹理坐标(UV坐标)
};
uniform Triangle tri[2];

struct hitRecord {
	vec3 Normal;
	vec3 Pos;
	vec3 albedo;
	int materialIndex;
};
hitRecord rec;

// 返回值：ray到球交点的距离
float hitSphere(Sphere s, Ray r);
float hitTriangle(Triangle tri, Ray r);
bool hitWorld(Ray r);
vec3 shading(Ray r);

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
// •计算Ray与三角形所在平面的交点。
// •判断Ray与三角形所在平面的交点是否在三角形内部。
float hitTriangle(Triangle tri, Ray r) {
	// 找到三角形所在平面法向量
    // 计算三角形平面法向量（使用两条边做叉乘）
    vec3 A = tri.v1 - tri.v0;  // 边向量v0->v1
    vec3 B = tri.v2 - tri.v0;  // 边向量v0->v2
    vec3 N = normalize(cross(A, B)); // 平面法向量

	// 判断光线是否与平面平行（方向向量与法线垂直）
	// Ray与平面平行，没有交点
	if (dot(N, r.direction) == 0) return -1.0;
    
	// 计算光线与平面交点参数t（平面方程：N·P + D = 0）
    float D = -dot(N, tri.v0); // 平面常数项
    float t = -(dot(N, r.origin) + D) / dot(N, r.direction);
    if (t < 0) return -1.0; // 交点在光线起点后方
	
	// 计算交点，三维交点坐标
	vec3 pHit = r.origin + t * r.direction;
	
	// 使用三次叉乘法判断交点是否在三角形内部
    // 检查v0->v1边（判断pHit是否在边外侧）
	vec3 edge0 = tri.v1 - tri.v0;
	vec3 C0 = pHit - tri.v0;
	if (dot(N, cross(edge0, C0)) < 0) return -1.0;
	
	// 检查v1->v2边
	vec3 edge1 = tri.v2 - tri.v1;
	vec3 C1 = pHit - tri.v1;
	if (dot(N, cross(edge1, C1)) < 0) return -1.0;
	
	// 检查v2->v0边
	vec3 edge2 = tri.v0 - tri.v2;
	vec3 C2 = pHit - tri.v2;
	if (dot(N, cross(edge2, C2)) < 0) return -1.0;
	
	// 光线与Ray相交
	// 减去微小值防止精度误差产生的自相交问题（阴影痤疮）
	// return t;
	return t - 0.00001;
}

// 返回值：ray到球交点的距离
bool hitWorld(Ray r) {
	float dis = 100000; // 初始化最近交点的距离为一个大数
	bool ifHitSphere = false; // 是否击中球物体的标志
	bool ifHitTriangle = false; // 是否击中三角形的标志
	int hitSphereIndex; // 记录被击中的球体索引
	int hitTriangleIndex; // 记录被击中的三角形索引

	// 计算球
	// 这个循环的核心目的就是找到光线最先击中的（即最近的）球体。
	// 遍历4个球体（根据循环条件i < 4）
	for (int i = 0; i < 4; i++) {
		float dis_t = hitSphere(sphere[i], r);
		if (dis_t > 0 && dis_t < dis) {
			dis = dis_t;
			hitSphereIndex = i;
			ifHitSphere = true;
		}
	}

	// 计算Mesh
	// 这个循环的目的就会找到光线最先击中的（即最近的）的三角形。
	// 遍历2个三角形 
	for (int i = 0; i < 2; i++) {
		float dis_t = hitTriangle(tri[i], r);
		if (dis_t > 0 && dis_t < dis) {
			dis = dis_t;
			hitTriangleIndex = i;
			ifHitTriangle = true;
		}
	}

	// 判断是否击中三角形和球体
	if (ifHitTriangle) { 
		rec.Pos = r.origin + dis * r.direction;
		rec.Normal = vec3(0.0,1.0,0.0); // 平面法线始终垂直x-z平面
		rec.albedo = vec3(0.7,0.7,0.7); // 反照率（基础颜色）
		rec.materialIndex = 1; // 为金属
		return true;
	}
	else if (ifHitSphere) {
		rec.Pos = r.origin + dis * r.direction;
		rec.Normal = normalize(r.origin + dis * r.direction - sphere[hitSphereIndex].center); // 球心指向交点的向量并归一化，就是该点的法向量。
		rec.albedo = sphere[hitSphereIndex].albedo; // 反照率（基础颜色）
		rec.materialIndex = sphere[hitSphereIndex].materialIndex; // 材质类型索引
		return true;
	}
	else return false;
}

// 返回值：随机单位球内的点
// 生成单位球体内的随机点（用于漫反射方向采样）
// 也就是，生成单位球内随机向量
// 为漫反射和金属反射提供随机方向采样
vec3 random_in_unit_sphere() {
	vec3 p;
	do {
		// rand()返回[0,1)区间的随机数。
		// vec3(rand(), rand(), rand())创建每个分量在[0,1)区间的随机向量。
		// 2.0 * vec3(rand(), rand(), rand())将每个分量乘以2，将范围扩大到[0,2)。
		// 再减去vec3(1)，将平移到[-1,1)范围内。
		// 最后取p的模长，如果模长大于1，则重新生成。
		p = 2.0 * vec3(rand(), rand() ,rand()) - vec3(1, 1, 1);
	} while (dot(p, p) >= 1.0); // 确保点在单位球内，点的模长小于等于1。
	return p;
}

vec3 diffuseReflection(vec3 Normal) {
	return normalize(Normal + random_in_unit_sphere());
}

vec3 metalReflection(vec3 rayIn, vec3 Normal) {
	vec3 r = rayIn - 2 * dot(rayIn, Normal) * Normal; // 完全反射
	return normalize(r + 0.35* random_in_unit_sphere()); // 添加随机扰动
}

// 返回值：颜色
vec3 shading(Ray r) {
	vec3 color = vec3(1.0,1.0,1.0);
	bool hitAnything = false;

	// 光线追踪次数为20次
	for (int i = 0; i < 100; i++) {
		if (hitWorld(r)) {
			// 更新光线起点
			// 交点成为新的光线起点
			r.origin = rec.Pos;
			if(rec.materialIndex == 0)
				r.direction = diffuseReflection(rec.Normal);
			else if(rec.materialIndex == 1)
				r.direction = metalReflection(r.direction, rec.Normal);
			color *= rec.albedo;
			hitAnything = true;
		}
		else {
			float t = 0.5*(r.direction.y + 1.0);
			color *= (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
			break;
		}
	}
	//if(!hitAnything) color = vec3(0.0, 0.0, 0.0);
	return color;
}









