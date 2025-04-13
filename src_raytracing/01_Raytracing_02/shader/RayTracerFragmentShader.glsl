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

uniform float randOrigin;

// 随机数种子
uint wseed;

// 随机数生成器
float randcore(uint seed) {
	seed = (seed ^ uint(61)) ^ (seed >> uint(16));
	seed *= uint(9);
	seed = seed ^ (seed >> uint(4));
	seed *= uint(0x27d4eb2d);
	wseed = seed ^ (seed >> uint(15));
	return float(wseed) * (1.0 / 4294967296.0);
}

// 随机数生成器
float rand() {
	return randcore(wseed);
}

// 采样历史帧的纹理采样器
uniform sampler2D historyTexture; // 历史帧的纹理

void main() {
	wseed = uint(randOrigin * float(6.95857) * (TexCoords.x * TexCoords.y));
	//if (distance(TexCoords, vec2(0.5, 0.5)) < 0.4)
	//	FragColor = vec4(rand(), rand(), rand(), 1.0);
	//else
	//	FragColor = vec4(0.0, 0.0, 0.0, 1.0);

	// 获取历史帧信息
	vec3 hist = texture(historyTexture, TexCoords).rgb;

	vec3 rayDir = normalize(camera.leftbottom + (TexCoords.x * 2.0 * camera.halfW) * camera.right + (TexCoords.y * 2.0 * camera.halfH) * camera.up);

	float radius = 1.2;
	vec3 sphereCenter = vec3(0.0, 0.0, 0.0);
	vec3 oc = camera.camPos - sphereCenter;
	float a = dot(rayDir, rayDir);
	float b = 2.0 * dot(oc, rayDir);
	float c = dot(oc, oc) - radius * radius;
	float discriminant = b * b - 4 * a * c;

	// 光线与球体相交判断 ========================================
	if (discriminant > 0.0)  // Δ>0表示有两个实根
	{
		// 取最近的交点
		float dis = (-b - sqrt(discriminant)) / (2.0 * a);
		// 有效正距离
		if (dis > 0.0) {
			// 法线
			vec3 N = normalize(camera.camPos + dis * rayDir - sphereCenter);
			// 光照方向
			vec3 LightDir = normalize(vec3(1.0, 1.0, 4.0));
			// 漫反射
			float dif = max(dot(N, LightDir), 0.0);
			// 镜面反射
			float spec = pow(max(dot(N, LightDir), 0.0), 64);
			// 光照强度
			float lu = 0.1 + 0.5 * dif + 0.4 * spec;
			//FragColor = vec4(lu, lu, lu, 1.0);

			// 渐进式渲染的时域累积
			// 当前颜色 = 当前帧颜色 + 历史帧颜色
			// 循环开始LoopNum就加1，所以uniform LoopNum设置时已经是1了。之后每循环一次加1。
			// 当前颜色 = (1/N) * 新采样颜色 + ((N-1)/N) * 历史颜色
			// 其实就是每帧颜色累加，然后除以帧数N。
			vec3 curColor = (1.0 / float(camera.LoopNum))*vec3(rand(), rand(), rand()) 
							+ (float(camera.LoopNum -1)/float(camera.LoopNum))*hist;
			FragColor = vec4(curColor, 1.0);
		}
		else {
			// 交点在相机后方
			FragColor = vec4(0.0, 0.0, 0.0, 1.0);
		}
	}
	else
	{	
		// 无实根，光线未击中球体
		FragColor = vec4(0.0, 0.0, 0.0, 1.0);
	}
}





