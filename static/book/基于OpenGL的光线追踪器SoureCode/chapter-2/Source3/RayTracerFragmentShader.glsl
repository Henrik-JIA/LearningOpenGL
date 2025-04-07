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
uniform struct Camera camera;

struct Ray {
	vec3 origin;
	vec3 direction;
};

uniform float randOrigin;
uint wseed;
float rand(void);

struct Sphere {
	vec3 center;
	float radius;
};
uniform Sphere sphere[4];

struct hitRecord {
	vec3 Normal;
	vec3 Pos;
};
hitRecord rec;

// 返回值：ray到球交点的距离
float hitSphere(Sphere s, Ray r);
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

	Ray cameraRay;
	cameraRay.origin = camera.camPos;
	cameraRay.direction = normalize(camera.leftbottom + (TexCoords.x * 2.0 * camera.halfW) * camera.right + (TexCoords.y * 2.0 * camera.halfH) * camera.up);

	if (hitWorld(cameraRay)) {
		vec3 N = rec.Normal;
		vec3 LightDir = normalize(vec3(1.0, 1.0, 4.0));
		float dif = max(dot(N, LightDir), 0.0);
		float spec = pow(max(dot(N, LightDir), 0.0), 64);
		float lu = 0.1 + 0.5 * dif + 0.4 * spec;
		FragColor = vec4(lu, lu, lu, 1.0);
	}
	else {
		FragColor = vec4(0.0, 0.0, 0.0, 1.0);
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

// 返回值：ray到球交点的距离
bool hitWorld(Ray r) {
	float dis = 100000;
	bool hitAnything = false;
	int hitSphereIndex;
	for (int i = 0; i < 4; i++) {
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



