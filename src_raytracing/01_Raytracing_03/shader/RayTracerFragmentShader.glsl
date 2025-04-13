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

// 随机数初值
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

// 返回值：是否击中场景
bool hitWorld(Ray r);

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

	// 生成光线
	Ray cameraRay; // 相机光线
	cameraRay.origin = camera.camPos; // 光线原点
	cameraRay.direction = normalize(camera.leftbottom + 
									(TexCoords.x * 2.0 * camera.halfW) * camera.right + 
									(TexCoords.y * 2.0 * camera.halfH) * camera.up); // 光线方向

	// 判断光线是否击中场景，如果击中，则计算光照
	if (hitWorld(cameraRay)) {
		vec3 N = rec.Normal;        // 表面法线
		vec3 LightDir = normalize(vec3(1.0, 1.0, 4.0)); // 光源方向（世界坐标）
		vec3 ViewDir = normalize(-cameraRay.direction); // 视线方向
		vec3 ReflectDir = reflect(-LightDir, N);        // 反射光方向
		
		float dif = max(dot(N, LightDir), 0.0);    // 漫反射强度
		float spec = pow(max(dot(ReflectDir, ViewDir), 0.0), 16); // 镜面反射强度
		
		float lu = 0.1 + 0.5 * dif + 0.4 * spec;   // 环境光 + 漫反射 + 镜面反射
		// 直接输出光照结果
		// FragColor = vec4(lu, lu, lu, 1.0);         // 单色光照结果
	
		// 渐进式渲染的时域累积光照结合随机数
		// 当前颜色 = 当前帧颜色 + 历史帧颜色
		// 循环开始LoopNum就加1，所以uniform LoopNum设置时已经是1了。之后每循环一次加1。
		// 当前颜色 = (1/N) * 新采样颜色 + ((N-1)/N) * 历史颜色
		// 其实就是每帧颜色累加，然后除以帧数N。
		vec3 curColor = (1.0 / float(camera.LoopNum))*vec3(lu * rand(), lu * rand(), lu * rand()) 
						+ (float(camera.LoopNum -1)/float(camera.LoopNum))*hist;
		FragColor = vec4(curColor, 1.0);
	
	}
	else {
		FragColor = vec4(0.0, 0.0, 0.0, 1.0); // 输出黑色
	}

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
	}else{
		return -1.0;
	}
}

// 返回值：是否击中场景
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



