#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <map>

#include <tool/shader.h>
#include <tool/camera.h>
#include <geometry/BoxGeometry.h>
#include <geometry/PlaneGeometry.h>
#include <geometry/SphereGeometry.h>

#define STB_IMAGE_IMPLEMENTATION
#include <tool/stb_image.h>

#include <tool/Gui.h>

#include <tool/model.h>

// 着色器代码
const char *light_sphere_vert = R"(

#version 330 core
layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TexCoords;
out vec2 outTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
  gl_Position = projection * view * model * vec4(Position, 1.0f);
  outTexCoord = TexCoords;
}

)";

const char *light_sphere_frag = R"(

#version 330 core
out vec4 FragColor;
in vec2 outTexCoord;

uniform vec3 lightColor;

void main() {
  FragColor = vec4(lightColor, 1.0);
}

)";

const char *light_source_vert = R"(
#version 330 core
layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TexCoords;

out vec2 outTexCoord;
out vec3 outNormal;
out vec3 outFragPos;

uniform float factor;

uniform float uvScale;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {

  gl_Position = projection * view * model * vec4(Position, 1.0f);

  outFragPos = vec3(model * vec4(Position, 1.0));

  outTexCoord = TexCoords * uvScale;
  // 解决不等比缩放，对法向量产生的影响
  outNormal = mat3(transpose(inverse(model))) * Normal;
}



)";

const char *light_source_frag = R"(

#version 330 core
out vec4 FragColor;

// 定向光
struct DirectionLight {
  vec3 direction;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

// 点光源
struct PointLight {
  vec3 position;

  float constant;
  float linear;
  float quadratic;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

// 聚光灯
struct SpotLight {
  vec3 position;
  vec3 direction;
  float cutOff;
  float outerCutOff;

  float constant;
  float linear;
  float quadratic;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

#define NR_POINT_LIGHTS 4

uniform DirectionLight directionLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;

uniform sampler2D brickMap; // 贴图

in vec2 outTexCoord;
in vec3 outNormal;
in vec3 outFragPos;

uniform vec3 viewPos;
uniform float factor; // 变化值

vec3 CalcDirectionLight(DirectionLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
float LinearizeDepth(float depth, float near, float far);

void main() {

  vec3 viewDir = normalize(viewPos - outFragPos);
  vec3 normal = normalize(outNormal);

  // 定向光照
  vec3 result = CalcDirectionLight(directionLight, normal, viewDir);

  // // 点光源
  // for(int i = 0; i < NR_POINT_LIGHTS; i++) {
  //   result += CalcPointLight(pointLights[i], normal, outFragPos, viewDir);
  // }

  // // 聚光光源
  // result += CalcSpotLight(spotLight, normal, outFragPos, viewDir) 
  //           * vec3(texture(awesomeMap, outTexCoord));

  vec4 texMap = texture(brickMap, outTexCoord);

  vec4 color = vec4(result, 1.0) * texMap;

  FragColor = vec4(color);
}

// 计算定向光
vec3 CalcDirectionLight(DirectionLight light, vec3 normal, vec3 viewDir) {
  vec3 lightDir = normalize(light.direction);
  float diff = max(dot(normal, lightDir), 0.0);
  vec3 reflectDir = reflect(-lightDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

  // 合并
  vec3 ambient = light.ambient;
  vec3 diffuse = light.diffuse * diff;
  vec3 specular = light.specular * spec;

  return ambient + diffuse + specular;
}

// 计算点光源
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
  vec3 lightDir = normalize(light.position - fragPos);
    // 漫反射着色
  float diff = max(dot(normal, lightDir), 0.0);
    // 镜面光着色
  vec3 reflectDir = reflect(-lightDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    // 衰减
  float distance = length(light.position - fragPos);
  float attenuation = 1.0 / (light.constant + light.linear * distance +
    light.quadratic * (distance * distance));    
    // 合并结果
  vec3 ambient = light.ambient;
  vec3 diffuse = light.diffuse * diff;
  vec3 specular = light.specular * spec;
  ambient *= attenuation;
  diffuse *= attenuation;
  specular *= attenuation;
  return (ambient + diffuse + specular);
}

// 计算聚光灯
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
  vec3 lightDir = normalize(light.position - fragPos);
  float diff = max(dot(normal, lightDir), 0.0);
  vec3 reflectDir = reflect(-lightDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

  float distance = length(light.position - fragPos);
  float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

  float theta = dot(lightDir, normalize(-light.direction));
  float epsilon = light.cutOff - light.outerCutOff;
  float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

  vec3 ambient = light.ambient;
  vec3 diffuse = light.diffuse * diff;
  vec3 specular = light.specular * spec;

  ambient *= attenuation * intensity;
  diffuse *= attenuation * intensity;
  specular *= attenuation * intensity;
  return (ambient + diffuse + specular);
}

// 计算深度值
float LinearizeDepth(float depth, float near, float far) {
  float z = depth * 2.0 - 1.0;
  return (2.0 * near * far) / (far + near - z * (far - near));
}


)";


const char *frame_buffer_quad_vert = R"(

#version 330 core
layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TexCoords;
out vec2 outTexCoord;

void main() {
  gl_Position = vec4(Position.x, Position.y, 0.0f, 1.0f);
  outTexCoord = TexCoords;
}

)";

const char *frame_buffer_quad_frag = R"(

#version 330 core
out vec4 FragColor;
in vec2 outTexCoord;

uniform sampler2D screenTexture;

const float offset = 1.0 / 300.0;
void main() {
  // 正常
  FragColor = vec4(texture(screenTexture, outTexCoord).rgb, 1.0);

  // 反相
  // vec3 texColor = 1.0 - texture(screenTexture, outTexCoord).rgb;
  // FragColor = vec4(texColor, 1.0);

  // 灰度
  // vec3 texColor = texture(screenTexture, outTexCoord).rgb;
  // float average = 0.2126 * texColor.r + 0.7152 * texColor.g + 0.0722 * texColor.b;
  // FragColor = vec4(vec3(average), 1.0);

  // 图像后处理中的边缘检测效果，具体是通过卷积核进行图像卷积操作。
  // vec2 offsets[9] = vec2[] (vec2(-offset, offset), // 左上
  // vec2(0.0f, offset), // 正上
  // vec2(offset, offset), // 右上
  // vec2(-offset, 0.0f),   // 左
  // vec2(0.0f, 0.0f),   // 中
  // vec2(offset, 0.0f),   // 右
  // vec2(-offset, -offset), // 左下
  // vec2(0.0f, -offset), // 正下
  // vec2(offset, -offset)  // 右下
  // );

  // float kernel[9] = float[] (1.0, 1.0, 1.0, 1.0, -8.0, 1.0, 1.0, 1.0, 1.0);
  // vec3 sampleTex[9];
  // for(int i = 0; i < 9; i++) {
  //   sampleTex[i] = vec3(texture(screenTexture, outTexCoord.st + offsets[i]));
  // }

  // vec3 col = vec3(0.0);
  // for(int i = 0; i < 9; i++)
  //   col += sampleTex[i] * kernel[i];

  // FragColor = vec4(col, 1.0);

}

)";

const char *cube_map_vert = R"(

#version 330 core
layout(location = 0) in vec3 Position;

out vec3 outTexCoord;

uniform mat4 view;
uniform mat4 projection;

void main() {
	outTexCoord = Position;
	vec4 pos = projection * view * vec4(Position, 1.0);
	gl_Position = pos.xyww;
}

)";

const char *cube_map_frag = R"(

#version 330 core
out vec4 FragColor;
in vec3 outTexCoord;

uniform samplerCube skyboxTexture;

void main() {
  FragColor = vec4(texture(skyboxTexture, outTexCoord).rgb, 1.0);
  // FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}

)";

// 反射物体着色器
const char *reflect_object_vert = R"(
#version 330 core
layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 TexCoords;


out vec2 oTexCoord;
out vec3 oNormal;
out vec3 oPosition;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    oTexCoord = TexCoords; // 纹理坐标
    oNormal = mat3(transpose(inverse(model))) * Normal; // 法向量
    oPosition = vec3(model * vec4(Position, 1.0)); // 物体位置
    gl_Position = projection * view * model * vec4(Position, 1.0);
}
)";

const char *reflect_object_frag = R"(
#version 330 core
out vec4 FragColor;

in vec2 oTexCoord;
in vec3 oNormal;
in vec3 oPosition;

uniform vec3 cameraPos;
uniform samplerCube cubeTexture;
uniform vec3 objectColor; 

void main()
{             
    vec3 viewDir = normalize(oPosition - cameraPos); // 相机位置
    vec3 R = reflect(viewDir, normalize(oNormal)); // 反射方向
    vec3 cubeMapColor = texture(cubeTexture, R).rgb; // 立方体贴图颜色，R当UV坐标来用。
    FragColor = vec4(cubeMapColor + objectColor, 1.0);
}
)";

// 折射物体着色器
const char *refract_object_vert = R"(
#version 330 core
layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 TexCoords;


out vec2 oTexCoord;
out vec3 oNormal;
out vec3 oPosition;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    oTexCoord = TexCoords;
    oNormal = mat3(transpose(inverse(model))) * Normal;
    oPosition = vec3(model * vec4(Position, 1.0));
    gl_Position = projection * view * model * vec4(Position, 1.0);
}
)";

const char *refract_object_frag = R"(
#version 330 core
out vec4 FragColor;

in vec2 oTexCoord;
in vec3 oNormal;
in vec3 oPosition;

uniform vec3 cameraPos;
uniform samplerCube cubeTexture;
uniform vec3 objectColor;

void main() {
    float ratio = 1.0 / 1.52;
    vec3 I = normalize(oPosition - cameraPos);
    vec3 R = refract(I, normalize(oNormal), ratio);
    FragColor = vec4(texture(cubeTexture, R).rgb + objectColor, 1.0);
}
)";

// obj物体着色器
const char *objVertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TexCoords;

out VS_OUT {
    vec3 vsOutNormal;
    vec2 vsOutTexCoord;
    vec3 vsOutFragPos;
} vs_out;

// MVP矩阵
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
  // MVP矩阵
  mat4 mvp = projection * view * model;
  // 顶点位置
  gl_Position = mvp * vec4(Position, 1.0f);
  // 顶点大小
  gl_PointSize = 10.0f;

  // 片段位置（只需乘以model矩阵，转为世界坐标）
  vs_out.vsOutFragPos = vec3(model * vec4(Position, 1.0f));

  // 纹理坐标
  vs_out.vsOutTexCoord = TexCoords;

  // 法线矩阵（解决不等比缩放的模型，法向量不垂直于面）
  mat3 normalMatrix = mat3(transpose(inverse(model)));
  // 法向量
  vs_out.vsOutNormal = normalMatrix * Normal;

}
)";

const char *objGeometryShaderSource = R"(
#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {
    vec3 vsOutNormal;
    vec2 vsOutTexCoord;
    vec3 vsOutFragPos;
} gs_in[];

out vec3 outNormal;
out vec2 outTexCoord;
out vec3 outFragPos;

uniform float time;

vec4 explode(vec4 position, vec3 normal)
{
    float magnitude = 2.0;
    vec3 direction = normal * ((sin(time) + 1.0) / 2.0) * magnitude; 
    return position + vec4(direction, 0.0);
}

vec3 GetNormal()
{
    vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
    vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
    return normalize(cross(a, b));
}

void main() {    
    vec3 normal = GetNormal();

    outTexCoord = gs_in[0].vsOutTexCoord;
    outNormal = gs_in[0].vsOutNormal;
    outFragPos = gs_in[0].vsOutFragPos;
    // gl_Position = explode(gl_in[0].gl_Position, normal); // 这里必须使用面法线，而不是顶点法线，如果使用顶点法线，则效果会沿着顶点法线方向，就不会有裂开的效果。
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();
    
    outTexCoord = gs_in[1].vsOutTexCoord;
    outNormal = gs_in[1].vsOutNormal;
    outFragPos = gs_in[1].vsOutFragPos;
    // gl_Position = explode(gl_in[1].gl_Position, normal);
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();
    
    outTexCoord = gs_in[2].vsOutTexCoord;
    outNormal = gs_in[2].vsOutNormal;
    outFragPos = gs_in[2].vsOutFragPos;
    // gl_Position = explode(gl_in[2].gl_Position, normal);
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
    
    EndPrimitive();
}

)";

const char *objFragmentShaderSource =  R"(
#version 330 core

// 定义材质结构体，材质在各光照条件下的颜色情况
struct Material{
  vec3 ambientColor; // 环境光颜色
  sampler2D diffuseMap; // 漫反射颜色(纹理)
  sampler2D specularMap; // 高光颜色(纹理)
  float shininess; // 高光指数
};

// 平行光（定向光）
struct DirectionLight {
  vec3 color;
  // 平行光属性
  vec3 parallelLightDirection; // 平行光方向（也就是光照方向，只是不需要计算每一个着色点指向光源的向量了，直接使用光照方向，因为平行光的光源方向是固定的，所有着色点都使用同一个光照方向）

  // 光强度属性
  vec3 ambientStrength; // 环境光强度
  vec3 diffuseStrength; // 漫反射强度
  vec3 specularStrength; // 镜面反射强度
};

// 点光源
struct PointLight {
  vec3 position;
  vec3 color;

  // 光强度属性
  vec3 ambientStrength; // 环境光强度
  vec3 diffuseStrength; // 漫反射强度
  vec3 specularStrength; // 镜面反射强度

  // 点光源衰减项参数
  float attenuationConstant; // 衰减常数项
  float attenuationLinear; // 衰减一次项
  float attenuationQuadratic; // 衰减二次项
};

// 聚光灯
struct SpotLight {
  vec3 position;
  vec3 direction;
  vec3 color;

  // 聚光灯属性
  float cutOff; // 聚光灯的切光角，是cos值
  float outerCutOff; // 聚光灯的外切光角，是cos值  

  // 光强度属性
  vec3 ambientStrength; // 环境光强度
  vec3 diffuseStrength; // 漫反射强度
  vec3 specularStrength; // 镜面反射强度

  // 点光源衰减项参数
  float attenuationConstant; // 衰减常数项
  float attenuationLinear; // 衰减一次项
  float attenuationQuadratic; // 衰减二次项
};

#define NR_POINT_LIGHTS 4

uniform Material material;
uniform DirectionLight directionLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;

// 输出变量
out vec4 FragColor;

// 输入变量
in vec2 outTexCoord; // 纹理坐标
in vec3 outNormal; // 顶点法线
in vec3 outFragPos; // 片段位置

// 相机位置
uniform vec3 viewPos;

// 定向光计算
vec3 CalcDirectionLight(DirectionLight light, vec3 normal, vec3 viewDir);
// 点光源计算
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
// 聚光灯计算
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
  // 物体颜色
  vec4 objectColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
  // 视线方向
  vec3 viewDir = normalize(viewPos - outFragPos);
  // 法线
  vec3 normal = normalize(outNormal);

  // 定向光照
  vec3 result = CalcDirectionLight(directionLight, normal, viewDir);

  // 点光源
  // for(int i = 0; i < NR_POINT_LIGHTS; i++) {
  //   result += CalcPointLight(pointLights[i], normal, outFragPos, viewDir);
  // }

  // // 聚光光源
  // result += CalcSpotLight(spotLight, normal, outFragPos, viewDir) 
  //           * vec3(texture(awesomeMap, outTexCoord));

  // 与物体自身颜色相乘
  result = result * vec3(objectColor);

  // 最终颜色
  FragColor = vec4(result, 1.0);

}


// 计算定向光（参数：定向光源结构体，法向量，视线方向）
// 平行光（太阳）不涉及光学衰减
vec3 CalcDirectionLight(DirectionLight light, vec3 normal, vec3 viewDir) {
  // 纹理颜色采样
  // 漫反射纹理采样颜色
  vec3 diffuseTextureColor = vec3(texture(material.diffuseMap, outTexCoord));
  // 高光纹理采样颜色
  vec3 specularTextureColor = vec3(texture(material.specularMap, outTexCoord));

  // 环境光项
  vec3 ambient = light.ambientStrength * diffuseTextureColor;

  // 漫反射项
  vec3 lightDir = normalize(light.parallelLightDirection); // 平行光方向
  float diff = max(dot(normal, lightDir), 0.0); // 计算平行光方向向量与法线的角度
  vec3 diffuse = light.diffuseStrength * diff * diffuseTextureColor;

  // 高光项（镜面反射）
  vec3 reflectDir = reflect(-lightDir, normal); // 计算反射方向
  // 计算镜面光强度（幂次项shininess越大，高光范围约集中）
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
  vec3 specular = light.specularStrength * spec * specularTextureColor;

  // 环境光+漫反射+镜面光
  return ambient + diffuse + specular * light.color;
}

// 计算点光源（参数：点光源结构体，法线，片元位置，视线方向）
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
  // 纹理颜色采样
  // 漫反射纹理采样颜色
  vec3 diffuseTextureColor = vec3(texture(material.diffuseMap, outTexCoord));
  // 高光纹理采样颜色
  vec3 specularTextureColor = vec3(texture(material.specularMap, outTexCoord));

  // 环境光项
  vec3 ambient = light.ambientStrength * diffuseTextureColor;

  // 漫反射项
  vec3 lightDir = normalize(light.position - fragPos);
  float diff = max(dot(normal, lightDir), 0.0);
  vec3 diffuse = light.diffuseStrength * diff * diffuseTextureColor;

  // 镜面反射项
  vec3 reflectDir = reflect(-lightDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
  vec3 specular = light.specularStrength * spec * specularTextureColor;

  // 衰减项
  float distanceToLight = length(light.position - outFragPos); // 光源距离
  float attenuation = 1.0 / (light.attenuationConstant + light.attenuationLinear * distanceToLight + 
                  light.attenuationQuadratic * (distanceToLight * distanceToLight));  
  ambient *= attenuation;
  diffuse *= attenuation;
  specular *= attenuation;

  // 环境光+漫反射+镜面光
  return (ambient + diffuse + specular) * light.color;
}

// 计算聚光灯（参数：聚光灯结构体，法线，片元位置，视线方向）
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
  // 纹理颜色采样
  // 漫反射纹理采样颜色
  vec3 diffuseTextureColor = vec3(texture(material.diffuseMap, outTexCoord));
  // 高光纹理采样颜色
  vec3 specularTextureColor = vec3(texture(material.specularMap, outTexCoord));

  // 环境光项
  vec3 ambient = light.ambientStrength * diffuseTextureColor;

  // 漫反射项
  vec3 lightDir = normalize(light.position - fragPos);
  float diff = max(dot(normal, lightDir), 0.0);
  vec3 diffuse = light.diffuseStrength * diff * diffuseTextureColor;
  
  // 高光项
  vec3 reflectDir = reflect(-lightDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
  vec3 specular = light.specularStrength * spec * specularTextureColor;

  // 衰减项
  float distanceToLight = length(light.position - outFragPos); // 光源距离
  float attenuation = 1.0 / (light.attenuationConstant + light.attenuationLinear * distanceToLight + 
                  light.attenuationQuadratic * (distanceToLight * distanceToLight));  
  ambient *= attenuation;
  diffuse *= attenuation;
  specular *= attenuation;

  // 聚光灯软化边缘
  // 聚光灯内切角到外加切角区间范围插值
  vec3 spotLightDirection = normalize(-light.direction); // 光源方向
  float theta = dot(lightDir, spotLightDirection); // 是cos值，为实际着色点与光源方向的夹角余弦值
  float Phi = light.cutOff; // 是cos值，切光角cos值
  float Phi_outer = light.outerCutOff; // 是cos值，外切光角cos值
  float epsilon = Phi - Phi_outer; // 是cos值，内切光角与外切光角的差值
  // 使用clamp函数，将intensity限制在0.0到1.0之间，就可以不用if-else判断了
  float intensity = clamp((theta - Phi_outer) / epsilon, 0.0, 1.0); // 是cos值，内切光角与外切光角的差值
  ambient *= intensity;
  diffuse *= intensity;
  specular *= intensity;
  
  // 环境光+漫反射+镜面光
  return (ambient + diffuse + specular) * light.color;
}
)";

// 法向量可视化着色器
const char *vs_normal_visualization = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out VS_OUT {
    vec3 normal;
} vs_out;

uniform mat4 view;
uniform mat4 model;

void main()
{
    mat3 normalMatrix = mat3(transpose(inverse(view * model)));
    vs_out.normal = vec3(vec4(normalMatrix * aNormal, 0.0));
    gl_Position = view * model * vec4(aPos, 1.0); 
}
)";

const char *gs_normal_visualization = R"(
#version 330 core
layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

in VS_OUT {
    vec3 normal;
} gs_in[];

const float MAGNITUDE = 0.01;

uniform mat4 projection;

void GenerateLine(int index)
{
    gl_Position = projection * gl_in[index].gl_Position;
    EmitVertex();
    gl_Position = projection * (gl_in[index].gl_Position + vec4(gs_in[index].normal, 0.0) * MAGNITUDE);
    EmitVertex();
    EndPrimitive();
}

void main()
{
    GenerateLine(0); // first vertex normal
    GenerateLine(1); // second vertex normal
    GenerateLine(2); // third vertex normal
}
)";

const char *fs_normal_visualization = R"(
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0, 1.0, 0.0, 1.0);
}

)";

// 实例化着色器
const char *instance_vert = R"(
#version 330 core
layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 offsets[100];

out vec2 oTexCoord;

void main() {
  oTexCoord = TexCoords;
  vec3 offset = offsets[gl_InstanceID];
  gl_Position = projection * view * model * vec4(Position + offset, 1.0f);
}
)";

const char *instance_frag = R"(
#version 330 core
out vec4 FragColor;

in vec2 oTexCoord;

void main() {
  FragColor = vec4(vec3(oTexCoord.x, oTexCoord.y, 1.0), 1.0);
}
)";


void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow *window, double xpos, double ypos); // 鼠标回调函数
void mouse_button_calback(GLFWwindow *window, int button, int action, int mods); // 鼠标按钮回调函数
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset); // 滚轮回调函数

unsigned int loadTexture(const char* path, GLint wrapS = GL_REPEAT, GLint wrapT = GL_REPEAT); // 加载纹理
unsigned int loadCubemap(vector<std::string> faces);

void drawSkyBox(Shader &shader, BoxGeometry &geometry, unsigned int &cubeMap);

int SCREEN_WIDTH = 800;
int SCREEN_HEIGHT = 600;
// int SCREEN_WIDTH = 1600;
// int SCREEN_HEIGHT = 1200;

// camera value
glm::vec3 cameraPos = glm::vec3(0.0f, 1.0f, 10.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// delta time
float deltaTime = 0.0f;
float lastTime = 0.0f;

// 鼠标上一帧的位置
bool isMousePressed = false;
bool isRightMousePressed = false;
double lastX = SCREEN_WIDTH / 2.0f; // 鼠标上一帧的位置
double lastY = SCREEN_HEIGHT / 2.0f;

Camera camera(glm::vec3(0.0, 1.0, 10.0));

unsigned int texColorBuffer, renderBuffer;

using namespace std;

int main()
{
  // 初始化GLFW
  glfwInit();
  // 片段着色器将作用域每一个采样点（采用4倍抗锯齿，则每个像素有4个片段（四个采样点））
  // glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // 窗口对象
  GLFWwindow *window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "LearnOpenGL", NULL, NULL);
  if (window == NULL)
  {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  // 初始化GLAD
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // 鼠标键盘事件
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); // 注册窗口变化监听
  glfwSetCursorPosCallback(window, mouse_callback); // 鼠标回调函数
  glfwSetMouseButtonCallback(window, mouse_button_calback); // 鼠标按钮回调函数
  glfwSetScrollCallback(window, scroll_callback); // 滚轮回调函数

  // -----------------------
  // 创建imgui上下文
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  ImGui::StyleColorsDark(); // 设置样式
  ImGui_ImplGlfw_InitForOpenGL(window, true); // 设置渲染平台
  ImGui_ImplOpenGL3_Init("#version 330"); // 设置渲染器后端
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
  // -----------------------

  // 设置视口
  glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  // 启用点大小
  glEnable(GL_PROGRAM_POINT_SIZE);
  // 深度测试（这里注释掉，因为我们使用了自定义的帧缓冲对象）
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  // 启用混合
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // 设置混合函数
  // 启用面剔除
  // glEnable(GL_CULL_FACE);
  // glCullFace(GL_BACK);

  // 创建着色器（包括顶点着色器、片段着色器、着色器程序、uniform设置）
  // 帧缓冲着色器
  Shader frameBufferShader = Shader::FromSource(frame_buffer_quad_vert, frame_buffer_quad_frag);
  // 天空盒子着色器
  Shader skyboxShader = Shader::FromSource(cube_map_vert, cube_map_frag);
  // phong物体着色器，光照对物体的影响
  Shader lightSourceShader = Shader::FromSource(light_source_vert, light_source_frag);
  // 模拟灯光位置的着色器，仅绘制纯色物体，不涉及光照，仅表示光照位置
  Shader lightObjectShader = Shader::FromSource(light_sphere_vert, light_sphere_frag);
  // 反射着色器
  Shader reflectShader = Shader::FromSource(reflect_object_vert, reflect_object_frag);
  // 折射着色器
  Shader refractShader = Shader::FromSource(refract_object_vert, refract_object_frag);
  // obj模型着色器
  Shader objectShader = Shader::FromSource(objVertexShaderSource, objFragmentShaderSource, objGeometryShaderSource);
  // 法向量可视化着色器
  Shader normalVisualizationShader = Shader::FromSource(vs_normal_visualization, fs_normal_visualization, gs_normal_visualization);
  // 实例化着色器
  Shader instanceShader = Shader::FromSource(instance_vert, instance_frag);


  float factor = 0.0;

  float fov = 45.0f; // 视锥体的角度
  
  int NR_POINT_LIGHTS = 4;

  ImVec4 clear_color = ImVec4(25.0 / 255.0, 25.0 / 255.0, 25.0 / 255.0, 0.1); // 25, 25, 25

  glm::vec3 lightPosition = glm::vec3(1.0, 2.5, 2.0); // 光照位置

  // 重建几何体
  PlaneGeometry frameGeometry(2.0, 2.0);               // 窗口平面
  BoxGeometry skyboxGeometry(1.0, 1.0, 1.0);           // 天空盒
  BoxGeometry containerGeometry(1.0, 1.0, 1.0);        // 箱子
  SphereGeometry sphereGeometry(0.1, 10.0, 10.0);      // 圆球

  // 实例化位置
  glm::vec3 translations[100];
  int index = 0;
  float offset = 0.3f;
  for (int y = -10; y < 10; y += 2)
  {
    for (int x = -10; x < 10; x += 2)
    {
      glm::vec3 translation;
      translation.x = (float)x / 10.0f + offset;
      translation.y = (float)y / 10.0f + offset;
      translation.z = 0.0;
      translations[index++] = translation;
    }
  }
  // 设置实例化位置
  instanceShader.use();
  for (unsigned int i = 0; i < 100; i++)
  {
    instanceShader.setVec3("offsets[" + std::to_string(i) + "]", translations[i]);
  }

  // 纹理加载
  unsigned int woodMap = loadTexture("../static/texture/wood.png", GL_REPEAT, GL_REPEAT);                         // 地面
  unsigned int brickMap = loadTexture("../static/texture/brick_diffuse.jpg", GL_REPEAT, GL_REPEAT);               // 砖块
  unsigned int grassMap = loadTexture("../static/texture/blending_transparent_window.png", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE); // 草丛

  // 加载模型（nanosuit）
  Model ourModel("../static/model/nanosuit/nanosuit.obj");

  // 点光源的位置
  glm::vec3 pointLightPositions[] = {
      glm::vec3(3.0f, 1.5f, 3.0f),    // 右前方
      glm::vec3(-3.0f, 2.5f, -3.0f),  // 左后方
      glm::vec3(-5.0f, 1.8f, 2.0f),   // 左侧更远处
      glm::vec3(4.0f, 2.2f, -2.0f)};  // 右后方
  // 点光源颜色
  glm::vec3 pointLightColors[] = {
      glm::vec3(1.0f, 0.0f, 0.0f),
      glm::vec3(1.0f, 0.0f, 1.0f),
      glm::vec3(0.0f, 0.0f, 1.0f),
      glm::vec3(0.0f, 1.0f, 0.0f)};
  // 草的位置
  vector<glm::vec3> grassPositions{
      glm::vec3(-1.5f, 0.5f, -0.48f),
      glm::vec3(1.5f, 0.5f, 0.51f),
      glm::vec3(0.0f, 0.5f, 0.7f),
      glm::vec3(-0.3f, 0.5f, -2.3f),
      glm::vec3(0.5f, 0.5f, -0.6f)};
  // 天空盒
  vector<string> faces{
      "../static/texture/skybox/right.jpg",  // +X 右
      "../static/texture/skybox/left.jpg",   // -X 左
      "../static/texture/skybox/top.jpg",    // +Y 上
      "../static/texture/skybox/bottom.jpg", // -Y 下
      "../static/texture/skybox/front.jpg",  // +Z 前
      "../static/texture/skybox/back.jpg"    // -Z 后
      };

  // 加载立方体贴图
  unsigned int cubeMapTexture = loadCubemap(faces);

  // use framebuffer 使用帧缓存
  // ---------------------------------------------------------
  // 1. 创建帧缓存对象
  unsigned int framebuffer; 
  glGenFramebuffers(1, &framebuffer); // 创建帧缓冲
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);// 绑定自定义帧缓存
  // 进行渲染操作

  // 2. 创建颜色附件(纹理)
  // unsigned int texColorBuffer; // 定义在全局区域了，当根据窗口大小了好调整。
  glGenTextures(1, &texColorBuffer); // 生成纹理
  glBindTexture(GL_TEXTURE_2D, texColorBuffer); // 绑定纹理
  // 设置纹理参数(大小与屏幕相同)
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL); // 设置纹理数据
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // 设置纹理过滤，GL_LINEAR表示线性过滤，GL_TEXTURE_MIN_FILTER表示缩小过滤
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // 设置纹理过滤，GL_LINEAR表示线性过滤，GL_TEXTURE_MAG_FILTER表示放大过滤
  // glBindTexture(GL_TEXTURE_2D, 0); // 解绑纹理（最后通过解绑帧缓冲对象时，顺带也解绑了纹理）

  // 3. 将它附加到当前绑定的帧缓冲对象
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0); // 将颜色纹理附加到当前绑定的帧缓冲对象

  // 4. 创建渲染缓冲对象(用于深度和模板测试)
  // unsigned int renderBuffer; // 定义在全局区域了，当根据窗口大小了好调整。
  glGenRenderbuffers(1, &renderBuffer); // 生成渲染缓冲
  glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer); // 绑定渲染缓冲
  // 设置深度和模板缓冲
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCREEN_WIDTH, SCREEN_HEIGHT); // 设置渲染缓冲存储
  // glBindRenderbuffer(GL_RENDERBUFFER, 0); // 解绑渲染缓冲（最后通过解绑帧缓冲对象时，顺带也解绑了渲染缓冲）

  // 5. 将渲染缓冲附加到帧缓存对象的深度和模板附件上，渲染缓冲也是帧缓冲对象，区别在于帧缓冲可以进行采样，渲染缓冲不能进行采样
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderBuffer); // 将渲染缓冲对象附加到帧缓冲的深度和模板附件上

  // 6. 检查帧缓冲是否是完整
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    std::cout << "ERROR:Framebuffer is not complete!" << std::endl;
  }

  // 7. 解绑帧缓冲（将帧缓冲对象绑定到默认帧缓存上）
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  // 使用之前渲染的纹理进行绘制
  // ---------------------------------------------------------

  // 渲染循环
  while (!glfwWindowShouldClose(window))
  {
    // 处理输入
    processInput(window);

    // 计算时间差，用于计算帧率以限制相机因时间变化而移动过快
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastTime;
    lastTime = currentFrame;

    // 在标题中显示帧率信息
    // *************************************************************************
    int fps_value = (int)round(ImGui::GetIO().Framerate);
    int ms_value = (int)round(1000.0f / ImGui::GetIO().Framerate);

    std::string FPS = std::to_string(fps_value);
    std::string ms = std::to_string(ms_value);
    std::string newTitle = "OpenGL - " + ms + " ms/frame " + FPS;
    glfwSetWindowTitle(window, newTitle.c_str());
    // *************************************************************************
    // 开始ImGui框架
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    // *************************************************************************

    // 渲染指令
    // ...

    // ************************************************************************* 

    // 绑定帧缓冲，将帧缓冲对象绑定到当前绑定的帧缓冲上，这里的帧缓冲是自定义的帧缓冲
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glEnable(GL_DEPTH_TEST);

    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w); // 设置背景颜色
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 清除颜色缓冲和深度缓冲
    
    // ************************************************************************* 

    // 绘制天空盒
    drawSkyBox(skyboxShader, skyboxGeometry, cubeMapTexture);

    // ************************************************************************* 
    // 修改光源颜色
    glm::vec3 lightColor;
    lightColor.x = sin(glfwGetTime() * 2.0f);
    lightColor.y = sin(glfwGetTime() * 0.7f);
    lightColor.z = sin(glfwGetTime() * 1.3f);

    // 设置视图矩阵和投影矩阵
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 model = glm::mat4(1.0f);
    projection = glm::perspective(glm::radians(fov), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);

    // 反射着色器
    reflectShader.use(); // 激活反射着色器
    reflectShader.setMat4("view", view);
    reflectShader.setMat4("projection", projection);
    reflectShader.setVec3("cameraPos", camera.Position);
    reflectShader.setVec3("objectColor", glm::vec3(0.0, 0.0, 0.0));
    // reflectShader.setVec3("objectColor", glm::vec3(0.1, 0.1, 0.0));

    // 折射着色器
    refractShader.use(); // 激活折射着色器
    refractShader.setMat4("view", view);
    refractShader.setMat4("projection", projection);
    refractShader.setVec3("cameraPos", camera.Position);
    reflectShader.setVec3("objectColor", glm::vec3(0.0, 0.0, 0.0));
    // refractShader.setVec3("objectColor", glm::vec3(0.1, 0.0, 0.1));

    // 灯光着色器
    lightSourceShader.use(); // 激活灯光着色器
    factor = glfwGetTime(); // 时间因子
    lightSourceShader.setFloat("factor", -factor * 0.3);
    lightSourceShader.setMat4("view", view); // 设置视图矩阵和投影矩阵
    lightSourceShader.setMat4("projection", projection);
    lightSourceShader.setVec3("viewPos", camera.Position); // 设置相机位置

    // 设置平行光光照属性
    // glm::vec3 lightPos = glm::vec3(lightPosition.x * glm::sin(glfwGetTime()) * 2.0, lightPosition.y, lightPosition.z); // 动态位置
    glm::vec3 lightPos = glm::vec3(lightPosition.x, lightPosition.y, lightPosition.z); // 固定位置
    lightSourceShader.setVec3("directionLight.direction", lightPos); // 光源位置
    // 设置平行光光照属性
    lightSourceShader.setVec3("directionLight.ambient", 0.01f, 0.01f, 0.01f);
    lightSourceShader.setVec3("directionLight.diffuse", 0.9f, 0.9f, 0.9f); // 将光照调暗了一些以搭配场景
    lightSourceShader.setVec3("directionLight.specular", 1.0f, 1.0f, 1.0f);

    // 实例化着色器
    instanceShader.use();
    instanceShader.setMat4("view", view);
    instanceShader.setMat4("projection", projection);
    instanceShader.setMat4("model", model);

    // obj模型着色器
    objectShader.use();
    objectShader.setFloat("factor", -factor * 0.3);
    objectShader.setFloat("time", glfwGetTime()); // 这里禁止传入时间，让三角面不随时间变化
    objectShader.setMat4("view", view); // 设置视图矩阵和投影矩阵
    objectShader.setMat4("projection", projection);
    objectShader.setVec3("viewPos", camera.Position); // 设置相机位置
    // 设置平行光光照属性
    // glm::vec3 lightPos = glm::vec3(lightPosition.x * glm::sin(glfwGetTime()) * 2.0, lightPosition.y, lightPosition.z); // 动态位置
    // glm::vec3 lightPos = glm::vec3(lightPosition.x, lightPosition.y, lightPosition.z); // 固定位置
    objectShader.setVec3("directionLight.parallelLightDirection", lightPos); // 光源位置
    // 设置平行光光照属性
    objectShader.setVec3("directionLight.color", 1.0f, 1.0f, 1.0f);
    objectShader.setVec3("directionLight.ambientStrength", 0.5f, 0.5f, 0.5f);
    objectShader.setVec3("directionLight.diffuseStrength", 0.9f, 0.9f, 0.9f); // 将光照调暗了一些以搭配场景
    objectShader.setVec3("directionLight.specularStrength", 1.0f, 1.0f, 1.0f);

    // 绘制obj模型
    // --------------------------------------------------
    // 绘制第一个模型（nanosuit）
    // glm::mat4 modelMatrix = glm::mat4(1.0f);
    // modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.2f, -1.3f, 2.5f));
    // modelMatrix = glm::scale(modelMatrix, glm::vec3(0.25f));
    // objectShader.setMat4("model", modelMatrix);
    // ourModel.Draw(objectShader);

    // obj模型法向量可视化
    // --------------------------------------------------
    // normalVisualizationShader.use();
    // normalVisualizationShader.setMat4("view", view);
    // normalVisualizationShader.setMat4("projection", projection);
    // normalVisualizationShader.setMat4("model", modelMatrix);
    // ourModel.Draw(normalVisualizationShader);

    // --------------------------------------------------

    // 绘制圆球
    // ----------------------------------------------------------
    // 实例化着色器
    glBindVertexArray(sphereGeometry.VAO);
    instanceShader.use(); 
    glDrawElementsInstanced(GL_TRIANGLES, sphereGeometry.indices.size(), GL_UNSIGNED_INT, 0, 100);
    glBindVertexArray(0);

    // ----------------------------------------------------------
    // 绘制砖块
    // ----------------------------------------------------------
    // glBindVertexArray(containerGeometry.VAO); // 绑定砖块的VAO
    // glBindTexture(GL_TEXTURE_2D, brickMap); // 绑定砖块的纹理
    
    // // 第一个砖块（左侧）反射
    // reflectShader.use(); // 激活反射着色器
    // model = glm::mat4(1.0f);
    // model = glm::translate(model, glm::vec3(-3.0f, 2.0f, 0.0f)); // X轴-3位置
    // model = glm::rotate(model, glm::radians((float)glfwGetTime() * 20.0f), glm::vec3(1.0, 0.0, 0.0));
    // model = glm::scale(model, glm::vec3(2.0, 2.0, 2.0));    
    // reflectShader.setMat4("model", model);
    // glDrawElements(GL_TRIANGLES, containerGeometry.indices.size(), GL_UNSIGNED_INT, 0);

    // // 第二个砖块（中间）漫反射
    // lightSourceShader.use(); // 激活漫反射着色器
    // lightSourceShader.setFloat("uvScale", 1.0f);
    // model = glm::mat4(1.0f);
    // model = glm::translate(model, glm::vec3(0.0f, 2.0f, 0.0f));  // X轴0位置
    // model = glm::rotate(model, glm::radians((float)glfwGetTime() * 20.0f), glm::vec3(1.0, 0.0, 0.0));
    // model = glm::scale(model, glm::vec3(2.0, 2.0, 2.0));   
    // lightSourceShader.setMat4("model", model);
    // glDrawElements(GL_TRIANGLES, containerGeometry.indices.size(), GL_UNSIGNED_INT, 0);

    // // 第三个砖块（右侧）折射
    // refractShader.use(); // 激活折射着色器
    // model = glm::mat4(1.0f);
    // model = glm::translate(model, glm::vec3(3.0f, 2.0f, 0.0f));  // X轴+3位置
    // model = glm::rotate(model, glm::radians((float)glfwGetTime() * 20.0f), glm::vec3(1.0, 0.0, 0.0));
    // model = glm::scale(model, glm::vec3(2.0f));
    // refractShader.setMat4("model", model);
    // glDrawElements(GL_TRIANGLES, containerGeometry.indices.size(), GL_UNSIGNED_INT, 0);
    
    // glBindVertexArray(0); // 解绑VAO
    // ----------------------------------------------------------

    // 帧缓冲对象绘制
    // ************************************************************

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // 返回默认的帧缓冲对象
    glDisable(GL_DEPTH_TEST);

    // 上面已经清楚过一次了
    // glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT);

    //绘制创建的帧缓冲屏幕窗口
    frameBufferShader.use();

    glBindVertexArray(frameGeometry.VAO); // 绑定VAO
   
    glBindTexture(GL_TEXTURE_2D, texColorBuffer);

    glDrawElements(GL_TRIANGLES, frameGeometry.indices.size(), GL_UNSIGNED_INT, 0);

    glBindVertexArray(0); // 解绑VAO

    // ************************************************************
    
    // 渲染 gui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // 交换缓冲
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // 释放资源
  // 清理ImGui
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();


  frameGeometry.dispose();
  containerGeometry.dispose();
  skyboxGeometry.dispose();
  glfwTerminate();

  return 0;
}

// 窗口变动监听
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
  // 添加最小化状态判断
  if (width == 0 || height == 0) {
      return; // 窗口最小化时不更新视口和缓冲
  }

  glViewport(0, 0, width, height);
  SCREEN_WIDTH = width;
  SCREEN_HEIGHT = height;

  // 更新帧缓冲附件尺寸
  glBindTexture(GL_TEXTURE_2D, texColorBuffer);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  glBindTexture(GL_TEXTURE_2D, 0); // 解绑（也可以不写）

  glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCREEN_WIDTH, SCREEN_HEIGHT);
  glBindRenderbuffer(GL_RENDERBUFFER, 0); // 解绑（也可以不写）

}

// 键盘输入监听
void processInput(GLFWwindow *window)
{
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
  {
    glfwSetWindowShouldClose(window, true);
  }

  // 相机按键控制
  // 相机移动
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
  {
    camera.ProcessKeyboard(FORWARD, deltaTime);
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
  {
    camera.ProcessKeyboard(BACKWARD, deltaTime);
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
  {
    camera.ProcessKeyboard(LEFT, deltaTime);
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
  {
    camera.ProcessKeyboard(RIGHT, deltaTime);
  }
}

// 鼠标移动回调
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;  // 新增：当ImGui使用鼠标时跳过场景处理

    // 左键：视角旋转
    if (isMousePressed) {
        float rotationSpeed = 3.0f;
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // 反转Y轴

        lastX = xpos;
        lastY = ypos;

        // 设置旋转速度
        xoffset *= rotationSpeed;
        yoffset *= rotationSpeed;

        camera.ProcessMouseMovement(xoffset, yoffset);
        cameraPos = camera.Position;
        cameraFront = camera.Front;
    }
    // 右键：视角平移
    else if (isRightMousePressed) {
        float sensitivity = 0.02f;
        float xoffset = xpos - lastX;
        float yoffset = ypos - lastY;

        lastX = xpos;
        lastY = ypos;

        // glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));
        // glm::vec3 up = cameraUp;

        // 获取相机坐标系轴向量
        glm::vec3 front = glm::normalize(camera.Front);
        glm::vec3 right = glm::normalize(glm::cross(front, camera.WorldUp));
        glm::vec3 up = glm::normalize(glm::cross(right, front));

        // 在相机平面内平移
        cameraPos -= right * xoffset * sensitivity; // 向左移动
        cameraPos += up * yoffset * sensitivity; // 向上移动
        camera.Position = cameraPos;
    }
    // 更新初始位置
    else {
        lastX = xpos;
        lastY = ypos;
    }
}

// 鼠标按键回调
void mouse_button_calback(GLFWwindow *window, int button, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;  // 新增：当ImGui使用鼠标时跳过场景处理

  // 左键处理
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
      if (action == GLFW_PRESS) {
          isMousePressed = true;
          glfwGetCursorPos(window, &lastX, &lastY);
      } else if (action == GLFW_RELEASE) {
          isMousePressed = false;
      }
  }
  // 右键处理
  else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
      if (action == GLFW_PRESS) {
          isRightMousePressed = true;
          glfwGetCursorPos(window, &lastX, &lastY);
      } else if (action == GLFW_RELEASE) {
          isRightMousePressed = false;
      }
  }
}

// 滚轮回调
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;  // 新增：当ImGui使用鼠标时跳过场景处理

    const float baseSpeed = 0.5f;
    const float shiftMultiplier = 3.0f; // 按住Shift加速
    
    float actualSpeed = baseSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        actualSpeed *= shiftMultiplier;
    }

    cameraPos += cameraFront * (float)yoffset * actualSpeed;
    camera.Position = cameraPos;
}

// 加载纹理
unsigned int loadTexture(const char* path, GLint wrapS, GLint wrapT) {
    unsigned int textureID = 0;
    int outWidth, outHeight, outChannels;

    stbi_set_flip_vertically_on_load(true); // 图像y轴翻转
    outWidth = outHeight = outChannels = 0; // 初始化输出参数
    unsigned char* data = stbi_load(path, &outWidth, &outHeight, &outChannels, 0);
    if (data) {
        // 生成纹理ID
        glGenTextures(1, &textureID);
        // 绑定纹理
        glBindTexture(GL_TEXTURE_2D, textureID);

        // 设置纹理参数
        GLenum format = GL_RGB;
        if (outChannels == 1)       format = GL_RED;
        else if (outChannels == 3)  format = GL_RGB;
        else if (outChannels == 4)  format = GL_RGBA;

        // 设置纹理数据
        glTexImage2D(GL_TEXTURE_2D, 0, format, outWidth, outHeight, 0, 
                    format, GL_UNSIGNED_BYTE, data);
        // 生成mipmap
        glGenerateMipmap(GL_TEXTURE_2D);

        // 设置纹理环绕方式（设置纹理坐标超出[0,1]范围时的处理方式）
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
        // 设置纹理缩放时的采样方式（设置纹理被放大和缩小时，使用哪种采样方式）
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cerr << "Texture failed to load: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID; // 只返回textureID
}

unsigned int loadCubemap(vector<std::string> faces)
{
  unsigned int textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

  // 此处需要将y轴旋转关闭，若之前调用过loadTexture
  stbi_set_flip_vertically_on_load(false);

  int width, height, nrChannels;
  for (unsigned int i = 0; i < faces.size(); i++)
  {
    unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                   0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
      stbi_image_free(data);
      std::cout << "Load cubemap texture success: " << faces[i].c_str() << std::endl;
    }
    else
    {
      std::cout << "Cubemap texture failed to load at path: " << faces[i].c_str() << std::endl;
      stbi_image_free(data);
    }
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  return textureID;
}

void drawSkyBox(Shader& shader, BoxGeometry& geometry, unsigned int& cubeMap)
{
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_DEPTH_TEST);

    shader.use();
    glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // 移除平移分量
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);

    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
  
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
    glBindVertexArray(geometry.VAO);
    glDrawElements(GL_TRIANGLES, geometry.indices.size(), GL_UNSIGNED_INT, 0);
  
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);

}