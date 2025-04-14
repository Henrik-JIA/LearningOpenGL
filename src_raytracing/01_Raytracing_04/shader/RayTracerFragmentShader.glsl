#version 330 core
out vec4 FragColor;

// 纹理坐标
in vec2 TexCoords;

// 屏幕宽高
uniform int screenWidth;
uniform int screenHeight;

// 相机结构体
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

// 光线结构体
struct Ray {
	vec3 origin;
	vec3 direction;
};

uniform float randOrigin;
uint wseed;
float rand(void);

// 球体数量和球体结构体
#define MAX_SPHERES 10
uniform int sphereNum;
struct Sphere {
	vec3 center;
	float radius;
};
uniform Sphere sphere[MAX_SPHERES];

// 击中记录结构体
struct hitRecord {
	vec3 Normal; // 法向量
	vec3 Pos; // 交点
};
hitRecord rec;

// 返回值：ray到球交点的距离
float hitSphere(Sphere s, Ray r);
bool hitWorld(Ray r);
vec3 shading(Ray r);

// 采样历史帧的纹理采样器
uniform sampler2D historyTexture;

// 主函数
void main() {
	wseed = uint(randOrigin * float(6.95857) * (TexCoords.x * TexCoords.y));
	//if (distance(TexCoords, vec2(0.5, 0.5)) < 0.4)
	//	FragColor = vec4(rand(), rand(), rand(), 1.0);
	//else
	//	FragColor = vec4(0.0, 0.0, 0.0, 1.0);

	// 获取历史帧信息
	vec3 hist = texture(historyTexture, TexCoords).rgb;

	// 生成光线
	Ray cameraRay;
	cameraRay.origin = camera.camPos;
	cameraRay.direction = normalize(camera.leftbottom + (TexCoords.x * 2.0 * camera.halfW) * camera.right + (TexCoords.y * 2.0 * camera.halfH) * camera.up);

	// 计算当前颜色
	// 初始值：vec3(1.0)（纯白）
	// 每次碰撞：颜色值乘以0.8（衰减20%）
	// 最终颜色：1.0 × 0.8^N（N为实际反弹次数）
	vec3 curColor = shading(cameraRay); // 这里返回累计衰减后的颜色
	
	// 混合历史帧和当前颜色，实现均值计算
	curColor = (1.0 / float(camera.LoopNum))*curColor 
				+ (float(camera.LoopNum - 1) / float(camera.LoopNum))*hist;
	
	// 输出颜色
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
		if (dis > 0.0) return dis;
		else return -1.0;
	}
	else return -1.0;
}

// 返回值：是否击中物体
bool hitWorld(Ray r) {
	float dis = 100000;
	bool hitAnything = false;
	int hitSphereIndex;
	for (int i = 0; i < sphereNum && i < MAX_SPHERES; i++) {
		float dis_t = hitSphere(sphere[i], r);
		if (dis_t > 0 && dis_t < dis) {
			dis = dis_t;
			hitSphereIndex = i;
			hitAnything = true;
		}
	}
	if (hitAnything) {
		rec.Pos = r.origin + dis * r.direction;
		rec.Normal = normalize(r.origin + dis * r.direction - sphere[hitSphereIndex].center);
		return true;
	}
	else return false;
}

// 随机生成单位球内的点
vec3 random_in_unit_sphere() {
	vec3 p;
	do {
		p = 2.0 * vec3(rand(), rand() ,rand()) - vec3(1, 1, 1);
	} while (dot(p, p) >= 1.0);
	return p;
}

// 计算当前颜色
vec3 shading(Ray r) {
    vec3 color = vec3(1.0); // 初始颜色为白色
	bool hitAnything = false; // 是否击中物体
    
    // 光线最多反弹20次
    for (int i = 0; i < 20; i++) {
        if (hitWorld(r)) { // 光线击中物体
			vec3 N = rec.Normal; // 法向量
			vec3 LightDir = normalize(vec3(1.0, 1.0, 4.0)); // 光源方向
			
			// 只在第一次碰撞时计算直接光照
			if(i == 0)
			{
				// 漫反射：兰伯特余弦定律
				float dif = max(dot(N, LightDir), 0.0);
				// 镜面反射：布林-冯模型（简化版）
				float spec = pow(max(dot(N, LightDir), 0.0), 64); // 镜面反射系数
				color = color * (0.1 + 0.5 * dif + 0.4 * spec); // 累加直接光照
			}

			// 准备下一次光线追踪
            r.origin = rec.Pos; // 新光线起点设为交点位置
            // 新光线方向：法线方向 + 随机单位球向量（实现漫反射）
            r.direction = normalize(rec.Normal + random_in_unit_sphere());

			// 每次碰撞都进行衰减，累积衰减
			color *= 0.8; // 每次反射衰减20%能量

			// 光线击中物体
            hitAnything = true;
        }
        else {
            break; // 光线逃逸到天空则终止
        }
    }
    
    // 如果从未击中任何物体，返回黑色
    if(!hitAnything) color = vec3(0.0);
    return color;
}





