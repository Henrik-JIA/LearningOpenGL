#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

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

// 随机数种子
uniform float randOrigin;
uint wseed;
float rand(void);

// 球体结构体
struct Sphere {
	vec3 center;
	float radius;
	vec3 albedo;
	int materialIndex;
};
uniform Sphere sphere[4];

// 击中记录结构体
// 这里会将球体的反射率和材质类型传给hitRecord结构体
struct hitRecord {
	vec3 Normal;
	vec3 Pos;
	vec3 albedo;
	int materialIndex;
};
hitRecord rec;

// 返回值：ray到球交点的距离
float hitSphere(Sphere s, Ray r);
// 返回值：是否击中
bool hitWorld(Ray r); 
// 返回值：着色
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

	// 生成相机光线
	Ray cameraRay;
	cameraRay.origin = camera.camPos;
	cameraRay.direction = normalize(camera.leftbottom + (TexCoords.x * 2.0 * camera.halfW) * camera.right + (TexCoords.y * 2.0 * camera.halfH) * camera.up);

	// 计算颜色
	vec3 curColor = shading(cameraRay);

	// 当前帧颜色与历史帧颜色混合求平均
	curColor = (1.0 / float(camera.LoopNum))*curColor + (float(camera.LoopNum - 1) / float(camera.LoopNum))*hist;
	
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

// 返回值：是否击中
// 判断光线是否与场景中的物体相交
bool hitWorld(Ray r) {
    float dis = 100000;          // 初始化最近交点的距离为一个大数
    bool hitAnything = false;    // 是否击中物体的标志
    int hitSphereIndex;          // 记录被击中的球体索引
    
	// 这个循环的核心目的就是找到光线最先击中的（即最近的）球体。
	// 遍历前4个球体（根据循环条件i < 4）
	for (int i = 0; i < 4; i++) {
		// 检测光线与第i个球体的交点
		float dis_t = hitSphere(sphere[i], r);

		// 如果当前交点比之前记录的更近
		if (dis_t > 0 && dis_t < dis) {
            dis = dis_t;              // 更新最近距离
            hitSphereIndex = i;       // 记录当前球体索引
            hitAnything = true;       // 标记已击中物体
		}
	}

	// 判断是否击中
	if (hitAnything) {
		// 计算交点坐标：光线起点 + 方向 * 距离
		// 光线起点每次都不同，首次是相机位置出发。
		// 在光线追踪过程中每次光线反弹后起点都会变化
		// 这个计算就是起点沿着方向移动dis距离。
		rec.Pos = r.origin + dis * r.direction;

		// 计算法线：交点到球心的向量归一化
		// 球心指向交点的向量并归一化，就是该点的法向量。
		rec.Normal = normalize(rec.Pos - sphere[hitSphereIndex].center);
		
		// 记录材质属性
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
        p = 2.0 * vec3(rand(), rand(), rand()) - vec3(1);
    } while (dot(p, p) >= 1.0); // 确保点在单位球内，点的模长小于等于1。
    return p;
}

// 返回值：漫反射
// 漫反射方向生成（Lambertian反射）
vec3 diffuseReflection(vec3 Normal) {
	// 法线方向 + 随机扰动 = 漫反射方向
	return normalize(Normal + random_in_unit_sphere());
}

// 返回值：金属反射
// 金属反射（带粗糙度的镜面反射）
// 物理模型：微表面镜面反射（带粗糙度）
// ----------------------------------------------
// 理想镜面反射（用于计算反射后的光线方向）：rayIn - 2 * dot(rayIn, Normal) * Normal
// 物理过程分解：
// 原始入射：v = v_parallel法线分量 + v_perp切平面分量
// 仅反转法线分量，保持切平面分量：
// 反射过程：r = -v_parallel + v_perp 
//            = (v_parallel + v_perp) - 2v_parallel 
//            = v - 2v_parallel
// ----------------------------------------------
// 表面粗糙度：0.35 * random_in_unit_sphere() 添加随机扰动，0.35控制粗糙度（0.0为完美镜面）
// 表面越粗糙（α越大），反射越模糊
vec3 metalReflection(vec3 rayIn, vec3 Normal) {
	vec3 r = rayIn - 2 * dot(rayIn, Normal) * Normal; // 完全反射
	return normalize(r + 0.35* random_in_unit_sphere()); // 添加随机扰动
}

// 返回值：着色
vec3 shading(Ray r) {
	// 初始颜色为白色
	vec3 color = vec3(1.0,1.0,1.0);
	// 是否击中物体
	bool hitAnything = false;

	// 光线追踪次数为20次
	for (int i = 0; i < 20; i++) {
		// 是否击中物体
		if (hitWorld(r)) {
			// 更新光线起点
			// 交点成为新的光线起点
			r.origin = rec.Pos;

			// 根据材质类型选择反射方向
			// 这里rec.Normal初始法线是球心指向交点的向量并归一化，就是光线交点的法向量。
			if(rec.materialIndex == 0)
				r.direction = diffuseReflection(rec.Normal);
			else if(rec.materialIndex == 1)
				r.direction = metalReflection(r.direction, rec.Normal); // 参数1为光线方向，参数2为交点法向量
			
			// 累加颜色
			color *= rec.albedo;
			// 是否击中物体
			hitAnything = true;
		}
		else {
			break;
		}
	}
	if(!hitAnything) color = vec3(0.0, 0.0, 0.0);
	return color;
}





