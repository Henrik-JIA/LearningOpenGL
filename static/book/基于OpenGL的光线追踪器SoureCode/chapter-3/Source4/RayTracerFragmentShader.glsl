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
	vec3 albedo;
	int materialIndex;
};
uniform Sphere sphere[4];

struct Triangle {
	vec3 v0, v1, v2;
	vec3 n0, n1, n2;
	vec2 u0, u1, u2;
};
uniform Triangle tri[2];

uniform sampler2D texMeshVertex;
uniform int meshVertexNum;
uniform sampler2D texMeshFaceIndex;
uniform int meshFaceNum;

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
	return t - 0.00001;
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
	float face0Index = At(texMeshFaceIndex, float(index * 3));
	float face1Index = At(texMeshFaceIndex, float(index * 3 + 1));
	float face2Index = At(texMeshFaceIndex, float(index * 3 + 2));

	tri_t.v0.x = At(texMeshVertex, face0Index * 8.0);
	tri_t.v0.y = At(texMeshVertex, face0Index * 8.0 + 1.0);
	tri_t.v0.z = At(texMeshVertex, face0Index * 8.0 + 2.0);

	tri_t.v1.x = At(texMeshVertex, face1Index * 8.0);
	tri_t.v1.y = At(texMeshVertex, face1Index * 8.0 + 1.0);
	tri_t.v1.z = At(texMeshVertex, face1Index * 8.0 + 2.0);

	tri_t.v2.x = At(texMeshVertex, face2Index * 8.0);
	tri_t.v2.y = At(texMeshVertex, face2Index * 8.0 + 1.0);
	tri_t.v2.z = At(texMeshVertex, face2Index * 8.0 + 2.0);
	
	//tri_t.v0 = tri[0].v0;
	//tri_t.v1 = tri[0].v1;
	//tri_t.v2 = tri[0].v2;

	return tri_t;
}

vec3 getTriangleNormal(Triangle tri) {
	return normalize(cross(tri.v2 - tri.v0, tri.v1 - tri.v0));
}

// 返回值：ray到球交点的距离
bool hitWorld(Ray r) {
	float dis = 100000;
	bool ifHitSphere = false;
	bool ifHitTriangle = false;
	int hitSphereIndex;
	int hitTriangleIndex;
	// 计算球
	/*for (int i = 0; i < 4; i++) {
		float dis_t = hitSphere(sphere[i], r);
		if (dis_t > 0 && dis_t < dis) {
			dis = dis_t;
			hitSphereIndex = i;
			ifHitSphere = true;
		}
	}*/
	// 计算Mesh
	for (int i = 0; i < meshFaceNum / 3; i++) {
		float dis_t = hitTriangle(getTriangle(i), r);
		if (dis_t > 0 && dis_t < dis) {
			dis = dis_t;
			hitTriangleIndex = i;
			ifHitTriangle = true;
		}
	}

	if (ifHitTriangle) {
		rec.Pos = r.origin + dis * r.direction;
		rec.Normal = getTriangleNormal(getTriangle(hitTriangleIndex));
		rec.albedo = vec3(0.87,0.77,0.12);
		rec.materialIndex = 1;
		return true;
	}
	else if (ifHitSphere) {
		rec.Pos = r.origin + dis * r.direction;
		rec.Normal = normalize(r.origin + dis * r.direction - sphere[hitSphereIndex].center);
		rec.albedo = sphere[hitSphereIndex].albedo;
		rec.materialIndex = sphere[hitSphereIndex].materialIndex;
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
	return normalize(rayIn - 2 * dot(rayIn, Normal) * Normal + 0.35* random_in_unit_sphere());
}

vec3 shading(Ray r) {
	vec3 color = vec3(1.0,1.0,1.0);
	for (int i = 0; i < 20; i++) {
		if (hitWorld(r)) {
			r.origin = rec.Pos;
			/*
			if(rec.materialIndex == 0)
				r.direction = diffuseReflection(rec.Normal);
			else if(rec.materialIndex == 1)
				r.direction = metalReflection(r.direction, rec.Normal);
			color *= rec.albedo;*/
			
			vec3 lightColor = vec3(1.0,1.0,1.0);
			vec3 lightPos = vec3(-4.0, 4.0, -4.0);
			// ambient
			float ambientStrength = 0.1;
			vec3 ambient = ambientStrength * lightColor;

			// diffuse 
			vec3 norm = rec.Normal;
			vec3 lightDir = normalize(lightPos - rec.Pos);
			float diff = max(dot(norm, lightDir), 0.0);
			vec3 diffuse = diff * lightColor;
			float specularStrength = 0.5;

			// specular
			vec3 viewDir = normalize(r.direction - rec.Pos);
			vec3 reflectDir = reflect(-lightDir, norm);
			float spec = pow(max(dot(r.direction, reflectDir), 0.0), 32);
			vec3 specular = specularStrength * spec * lightColor;

			vec3 result = (ambient + diffuse + specular) * vec3(0.3, 0.7, 0.2);

			color = result;
			break;
		}
		else {
			float t = 0.5*(r.direction.y + 1.0);
			color *= (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
			break;
		}
	}
	return color;
}












