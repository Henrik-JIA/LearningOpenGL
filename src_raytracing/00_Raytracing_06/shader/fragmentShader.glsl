#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform int screenWidth;
uniform int screenHeight;

uniform vec3 lookat;

struct Camera {
	vec3 camPos;
	vec3 front;
	vec3 right;
	vec3 up;
	float halfH;
	float halfW;
	vec3 leftbottom;
};
uniform Camera camera;

uint wseed;
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


void main() {
	// 初始化随机数种子（根据纹理坐标），未使用
	wseed = uint(float(69557857) * (TexCoords.x * TexCoords.y));
	//if (distance(TexCoords, vec2(0.5, 0.5)) < 0.4)
	//	FragColor = vec4(rand(), rand(), rand(), 1.0);
	//else
	//	FragColor = vec4(0.0, 0.0, 0.0, 1.0);

	// 计算射线方向 ============================================
	// 相机左下角 + 水平偏移 + 垂直偏移
	vec3 rayDir = normalize( 
		camera.leftbottom // 相机成像平面左下角坐标
		+ (TexCoords.x * 2.0 * camera.halfW) * camera.right // 水平方向偏移量
		+ (TexCoords.y * 2.0 * camera.halfH) * camera.up // 垂直方向偏移量
	);
    // 原理：将屏幕UV坐标(TexCoords)转换为世界空间中的光线方向
    // - TexCoords.x范围[0,1]对应成像平面从左到右
    // - TexCoords.y范围[0,1]对应成像平面从下到上
    // - camera.halfW/halfH是成像平面半宽高
    // - camera.right/up是相机的右方向和上方向向量

	
	// 球体相交检测 ============================================
	float radius = 1.2; // 球半径
	vec3 sphereCenter = vec3(0.0, 0.0, 0.0); // 球心世界坐标
	vec3 oc = camera.camPos - sphereCenter; // 相机到球心的向量
	
	// 构建二次方程：t² + 2bt + c = 0
	float a = dot(rayDir, rayDir); // 光线方向点乘自身（通常为1，因已标准化）
	float b = 2.0 * dot(oc, rayDir); // 2倍相机-球心向量与光线方向的点积
	float c = dot(oc, oc) - radius * radius; // 相机到球心距离平方 - 半径平方
	float discriminant = b * b - 4 * a * c; // 判别式Δ，用于判断光线与球体是否有交点，b^2 - 4ac

	// 光线与球体相交判断 ========================================
	if (discriminant > 0.0) // Δ>0表示有两个实根
	{
		// 取最近的交点
		float dis = (-b - sqrt(discriminant)) / (2.0 * a);
		// 有效正距离
		if (dis > 0.0) 
		{
			vec3 N = normalize(
				camera.camPos  // 相机位置
				+ dis * rayDir // 沿光线方向移动dis距离
				- sphereCenter // 减去球心得到表面点相对球心的向量
			); // 法向量

			// 光照计算 ========================================
			vec3 LightDir = normalize(vec3(1.0, 1.0, 4.0)); // 光源方向

			// 漫反射：兰伯特余弦定律
			float dif = max(dot(N, LightDir), 0.0); // 漫反射系数
			// 镜面反射：布林-冯模型（简化版）
			float spec = pow(max(dot(N, LightDir), 0.0), 64); // 镜面反射系数
			// 合成光照：环境光(0.1) + 漫反射(0.5) + 镜面反射(0.4)
			float lu = 0.1 + 0.5 * dif + 0.4 * spec; // 光照强度
			FragColor = vec4(lu, lu, lu, 1.0); // 灰度显示光照强度
		}
		else 
		{
			// 交点在相机后方
			FragColor = vec4(0.0, 0.0, 0.0, 1.0); // 黑色背景
		}
	}
	else
	{	
		// 无实根，光线未击中球体
		FragColor = vec4(0.0, 0.0, 0.0, 1.0); // 黑色背景
	}
}



