#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <tool/Shader.h>
#include <tool/Camera.h>
#include <geometry/PlaneGeometry.h>
#include <geometry/BoxGeometry.h>
#include <geometry/SphereGeometry.h>

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <tool/stb_image.h>

#include <tool/Gui.h>

#include <tool/mesh.h>
#include <tool/model.h>

// 着色器代码
const char *vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TexCoords;

out vec3 outNormal;
out vec2 outTexCoord;
out vec3 outFragPos;

// MVP矩阵
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// 时间因子
uniform float factor;

// 纹理缩放
uniform float uvScale;

void main() {
  // MVP矩阵
  mat4 mvp = projection * view * model;
  // 顶点位置
  gl_Position = mvp * vec4(Position, 1.0f);
  // 顶点大小
  gl_PointSize = 10.0f;

  // 片段位置（只需乘以model矩阵，转为世界坐标）
  outFragPos = vec3(model * vec4(Position, 1.0f));

  // 纹理坐标
  outTexCoord = TexCoords / uvScale;

  // 法线矩阵（解决不等比缩放的模型，法向量不垂直于面）
  mat3 normalMatrix = mat3(transpose(inverse(model)));
  // 法向量
  outNormal = normalMatrix * Normal;

}
)";

const char *fragmentShaderSource =  R"(
#version 330 core

// 定义材质结构体，材质在各光照条件下的颜色情况
struct Material{
  vec3 ambientColor; // 环境光颜色
  sampler2D diffuseMap; // 漫反射纹理
  sampler2D specularMap; // 高光纹理
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
// 动态时间值（变化值）
uniform float factor;

float near = 0.1;
float far = 100.0;

uniform int objectType;
uniform sampler2D diffuseBrickMap; // 漫反射砖块纹理
uniform sampler2D diffuseWoodMap; // 漫反射木纹纹理
uniform sampler2D diffuseWindowMap; // 漫反射窗户纹理

// 定向光计算
vec3 CalcDirectionLight(DirectionLight light, vec3 normal, vec3 viewDir);
// 点光源计算
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
// 聚光灯计算
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
// 计算深度值（返回一个near与far之间的深度值）
float LinearizeDepth(float depth, float near, float far);

void main() {
  // 物体颜色
  vec4 objectColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
  // 视线方向
  vec3 viewDir = normalize(viewPos - outFragPos);
  // 法线
  vec3 normal = normalize(outNormal);

  // 定向光照
  vec3 result = CalcDirectionLight(directionLight, normal, viewDir);

  // 点光源
  for(int i = 0; i < NR_POINT_LIGHTS; i++) {
    result += CalcPointLight(pointLights[i], normal, outFragPos, viewDir);
  }

  // 与物体自身颜色相乘
  result = result * vec3(objectColor);

  vec4 texMap;
    if(objectType == 0) { // 地面
        texMap = texture(diffuseWoodMap, outTexCoord);
    } else if(objectType == 1) { // 砖块
        texMap = texture(diffuseBrickMap, outTexCoord);
    } else { // 窗户
        texMap = texture(diffuseWindowMap, outTexCoord);
    }

  if(texMap.a < 0.1)
        discard;
  
  // 最终颜色
  vec4 color = vec4(result, 1.0) * texMap;

  FragColor = vec4(color);

  // 渲染深度缓冲
  // FragColor = vec4(vec3(gl_FragCoord.z), 1.0);
  
  // 线性化深度值（将深度值转换为0-1之间的值，视角越远，越接近1，场景渲染越白）
  // float depth = LinearizeDepth(gl_FragCoord.z, near, far) / far;
  // FragColor = vec4(vec3(depth), 1.0);

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

// 计算深度值
float LinearizeDepth(float depth, float near, float far) {
  float z = depth * 2.0 - 1.0; // 转换为 NDC
  return (2.0 * near * far) / (far + near - z * (far - near));
}

)";

const char *light_object_vert = R"(
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

const char *light_object_frag = R"(
#version 330 core
out vec4 FragColor;
in vec2 outTexCoord;

uniform vec3 lightColor;

void main() {
  FragColor = vec4(lightColor, 1.0);
}
)";

using namespace std;

// 函数声明
void framebuffer_size_callback(GLFWwindow* window, int width, int height); // 窗口大小回调函数
void processInput(GLFWwindow *window); // 处理输入
void mouse_callback(GLFWwindow *window, double xpos, double ypos); // 鼠标回调函数
void mouse_button_calback(GLFWwindow *window, int button, int action, int mods); // 鼠标按钮回调函数
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset); // 滚轮回调函数
unsigned int loadTexture(const char* path, int& outWidth, int& outHeight, int& outChannels, GLint wrapS = GL_REPEAT, GLint wrapT = GL_REPEAT); // 加载纹理

// 屏幕宽高
int SCREEN_WIDTH = 800;
int SCREEN_HEIGHT = 600;

// 计算时间差
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// 当前时间因子
float factor = 0.0;

// 鼠标上一帧的位置
bool isMousePressed = false;
bool isRightMousePressed = false;
double lastX = SCREEN_WIDTH / 2.0f;
double lastY = SCREEN_HEIGHT / 2.0f;

// 视锥体的角度
float fov = 60.0f; 

// 背景颜色
ImVec4 clear_color = ImVec4(0.2f, 0.2f, 0.2f, 1.00f);

// 相机系统
Camera camera(SCREEN_WIDTH, SCREEN_HEIGHT, glm::vec3(0.0f, 1.0f, 6.0f)); // 相机类
glm::vec3 cameraPos = camera.Position; // 相机位置
glm::vec3 cameraFront = camera.Front; // 相机前向
glm::vec3 cameraUp = camera.Up; // 相机上向

// 材质属性
glm::vec3 materialAmbientColor = glm::vec3(1.0f, 0.5f, 0.31f);
int materialDiffuseMap = 0; // 漫反射颜色(纹理)
int materialSpecularMap = 1; // 高光颜色(纹理)
int shininess = 64; // 高光指数

// 光源属性（初始值）
// 平行光
// glm::vec3 lightDirection = glm::vec3(0.0, 0.0, -1.0); // 光照方向
glm::vec3 parallelLightPosition = glm::vec3(1.0, 2.5, 2.0); // 平行光光照位置
glm::vec3 parallelLightDirection = glm::vec3(0.0, 0.0, -1.0); // 平行光方向
glm::vec3 parallelLightColor = glm::vec3(1.0f, 0.5f, 0.0f); // 光照颜色橘黄色
glm::vec3 parallelLightAmbientStrength = glm::vec3(0.01f, 0.01f, 0.01f); // 环境光强度
glm::vec3 parallelLightDiffuseStrength = glm::vec3(0.2f, 0.2f, 0.2f); // 漫反射强度
glm::vec3 parallelLightSpecularStrength = glm::vec3(1.0f, 1.0f, 1.0f); // 镜面反射强度

// 点光源
glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f); // 光照颜色
glm::vec3 lightAmbientStrength = glm::vec3(0.5f, 0.5f, 0.5f); // 环境光强度
glm::vec3 lightDiffuseStrength = glm::vec3(0.8f, 0.8f, 0.8f); // 漫反射强度
glm::vec3 lightSpecularStrength = glm::vec3(1.0f, 1.0f, 1.0f); // 镜面反射强度
float lightAttenuationConstant = 1.0f; // 衰减常数项，常数项一般都是1.0
float lightAttenuationLinear = 0.09f; // 衰减一次项，对应50m距离
float lightAttenuationQuadratic = 0.032f; // 衰减二次项，对应50m距离

// 创建多个立方体初始位置
glm::vec3 cubePositions[] = {
    glm::vec3(1.0, 1.0, -1.0), 
    glm::vec3(-1.0, 0.5, 2.0), 
  };

// 多个立方体缩放比例
glm::vec3 cubeScales[] = {
    glm::vec3(2.0, 2.0, 2.0),
    glm::vec3(1.0, 1.0, 1.0),
};

// 点光源的位置
glm::vec3 pointLightPositions[] = {
    glm::vec3(3.7f, 0.2f, 1.5f),
    glm::vec3(2.3f, -3.3f, -4.0f),
    glm::vec3(-4.0f, 2.0f, -12.0f),
    glm::vec3(0.0f, 0.0f, -3.0f)};

glm::vec3 pointLightColors[] = {
    glm::vec3(1.0f, 1.0f, 1.0f),
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

int main()
{
  // 初始化GLFW
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // 创建窗口对象
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

  // 设置回调函数
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); // 窗口大小回调函数
  glfwSetCursorPosCallback(window, mouse_callback); // 鼠标回调函数
  glfwSetMouseButtonCallback(window, mouse_button_calback); // 鼠标按钮回调函数
  glfwSetScrollCallback(window, scroll_callback); // 滚轮回调函数

  // 初始化ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  ImGui::StyleColorsDark(); // 设置ImGui风格
  ImGui_ImplGlfw_InitForOpenGL(window, true); // 设置渲染平台
  ImGui_ImplOpenGL3_Init("#version 330"); // 设置渲染器后端
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

  // OpenGL配置
  glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT); // 设置视口
  glEnable(GL_PROGRAM_POINT_SIZE); // 启用点大小

  // 设置绘制模式
  // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  // 开启深度测试
  glEnable(GL_DEPTH_TEST);
  // 设置深度测试函数，默认是GL_LESS，表示深度测试通过的条件是：当前片段的深度值小于缓冲区的深度值
  glDepthFunc(GL_LESS);
  // glDepthFunc(GL_ALWAYS); // 总是通过深度测试

  // 启用混合
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // 设置混合函数

  // 启用面剔除
  glEnable(GL_CULL_FACE); 
  glCullFace(GL_FRONT);

  // 创建着色器（包括顶点着色器、片段着色器、着色器程序、uniform设置）
  // 物体着色器
  Shader sceneShader = Shader::FromSource(vertexShaderSource, fragmentShaderSource);
  // 灯光物体着色器
  Shader lightObjectShader = Shader::FromSource(light_object_vert, light_object_frag);

  // 获取uniform变量location
  // 物体着色器：
  // 时间因子
  GLint locFactor = sceneShader.getUniformLocation("factor");

  // MVP矩阵
  GLint locModel = sceneShader.getUniformLocation("model");
  GLint locView = sceneShader.getUniformLocation("view");
  GLint locProjection = sceneShader.getUniformLocation("projection");
  
  // 相机位置
  GLint locViewPos = sceneShader.getUniformLocation("viewPos");
  
  // 材质纹理属性位置
  GLint locMaterialAmbientColor = sceneShader.getUniformLocation("material.ambientColor");
  GLint locMaterialDiffuseWoodMap = sceneShader.getUniformLocation("material.diffuseWoodMap");
  GLint locMaterialDiffuseBrickMap = sceneShader.getUniformLocation("material.diffuseBrickMap");
  GLint locMaterialShininess = sceneShader.getUniformLocation("material.shininess");
  
  // 平行光源属性位置
  GLint locDirectionLightColor = sceneShader.getUniformLocation("directionLight.color");
  GLint locDirectionLightDirection = sceneShader.getUniformLocation("directionLight.parallelLightDirection");
  GLint locDirectionLightAmbientStrength = sceneShader.getUniformLocation("directionLight.ambientStrength");
  GLint locDirectionLightDiffuseStrength = sceneShader.getUniformLocation("directionLight.diffuseStrength");
  GLint locDirectionLightSpecularStrength = sceneShader.getUniformLocation("directionLight.specularStrength");

  // 点光源属性位置
  vector<GLint> locPointLightPosition;
  vector<GLint> locPointLightColor;
  vector<GLint> locPointLightAmbientStrength;
  vector<GLint> locPointLightDiffuseStrength;
  vector<GLint> locPointLightSpecularStrength;
  vector<GLint> locPointLightAttenuationConstant;
  vector<GLint> locPointLightAttenuationLinear;
  vector<GLint> locPointLightAttenuationQuadratic;
  for (unsigned int i = 0; i < 4; i++)
  {
    locPointLightPosition.push_back(sceneShader.getUniformLocation("pointLights[" + to_string(i) + "].position"));
    locPointLightColor.push_back(sceneShader.getUniformLocation("pointLights[" + to_string(i) + "].color"));
    locPointLightAmbientStrength.push_back(sceneShader.getUniformLocation("pointLights[" + to_string(i) + "].ambientStrength"));
    locPointLightDiffuseStrength.push_back(sceneShader.getUniformLocation("pointLights[" + to_string(i) + "].diffuseStrength"));
    locPointLightSpecularStrength.push_back(sceneShader.getUniformLocation("pointLights[" + to_string(i) + "].specularStrength"));
    locPointLightAttenuationConstant.push_back(sceneShader.getUniformLocation("pointLights[" + to_string(i) + "].attenuationConstant"));
    locPointLightAttenuationLinear.push_back(sceneShader.getUniformLocation("pointLights[" + to_string(i) + "].attenuationLinear"));
    locPointLightAttenuationQuadratic.push_back(sceneShader.getUniformLocation("pointLights[" + to_string(i) + "].attenuationQuadratic")); 
  }

  // 灯光物体着色器：
  // MVP矩阵位置
  GLint locLightModel = lightObjectShader.getUniformLocation("model");
  GLint locLightView = lightObjectShader.getUniformLocation("view");
  GLint locLightProjection = lightObjectShader.getUniformLocation("projection");

  // 重建几何体
  PlaneGeometry windowGeometry(1.0, 1.0);               // 窗户
  PlaneGeometry planeGeometry(10.0, 10.0, 10.0, 10.0); // 地面
  BoxGeometry boxGeometry(1.0, 1.0, 1.0); // 盒子
  SphereGeometry sphereGeometry(0.04, 10.0, 10.0); // 点光源位置显示

  // 纹理类型地址
  GLint locObjectType = sceneShader.getUniformLocation("objectType");

  // ************************************************************
  // 纹理加载
  unsigned int diffuseBrickMap, diffuseWoodMap, diffuseWindowMap;
  int width1, height1, channels1;
  int width2, height2, channels2;
  int width3, height3, channels3;
  // 纹理1
  diffuseBrickMap = loadTexture("../static/texture/brick_diffuse.jpg", width2, height2, channels2);
  // 纹理2
  diffuseWoodMap = loadTexture("../static/texture/wood.png", width1, height1, channels1);  
  // 纹理3
  diffuseWindowMap = loadTexture("../static/texture/blending_transparent_window.png", width3, height3, channels3, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE); // 草丛

  // ************************************************************
  // 渲染循环
  while (!glfwWindowShouldClose(window))
  {
    // 计算时间差，用于计算帧率以限制相机因时间变化而移动过快
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    // 处理输入（使用最新的deltaTime）
    processInput(window);
    // *************************************************************************
    // 在标题中显示帧率信息
    int fps_value = (int)round(ImGui::GetIO().Framerate);
    int ms_value = (int)round(1000.0f / ImGui::GetIO().Framerate);
    std::string FPS = std::to_string(fps_value);
    std::string ms = std::to_string(ms_value);
    std::string newTitle = "LearnOpenGL - " + ms + " ms/frame " + FPS;
    glfwSetWindowTitle(window, newTitle.c_str());
    // *************************************************************************
    // 开始ImGui框架
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    // *************************************************************************
    {
      ImGui::Begin("Hello, world!");
      // 设置固定控件宽度
      ImGui::PushItemWidth(200); // 固定为200像素宽
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4)); // 调整内边距
      // 帧率显示
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
      if (ImGui::CollapsingHeader("Display set"))
      {
        ImGui::ColorEdit3("clear color(background color)", (float*)&clear_color);
        ImGui::SliderFloat("fov", &fov, 15.0f, 90.0f);
        ImGui::SliderInt("SCREEN_WIDTH", &SCREEN_WIDTH, 100, 1920);
        ImGui::SliderInt("SCREEN_HEIGHT", &SCREEN_HEIGHT, 100, 1080);
      }

      if (ImGui::CollapsingHeader("Light property")) 
      {
        ImGui::ColorEdit3("Light_Color", (float*)&lightColor);
        // ImGui::SliderFloat3("Light Direction", glm::value_ptr(lightDirection), -1.0f, 1.0f);
        ImGui::ColorEdit3("Light_AmbientStrength", (float*)&lightAmbientStrength);
        ImGui::ColorEdit3("Light_DiffuseStrength", (float*)&lightDiffuseStrength);
        ImGui::ColorEdit3("Light_SpecularStrength", (float*)&lightSpecularStrength);
      }

      if (ImGui::CollapsingHeader("Material property"))
      {
        ImGui::ColorEdit3("Material_AmbientColor", (float*)&materialAmbientColor);
        // ImGui::ColorEdit3("Material_DiffuseColor", (float*)&materialDiffuseMap);
        // ImGui::ColorEdit3("Material_SpecularColor", (float*)&materialSpecularMap);
        ImGui::SliderInt("Material_Shininess", &shininess, 1, 256);
      }
      
      // 恢复样式设置（在最后一个控件之后）
      ImGui::PopStyleVar();
      ImGui::PopItemWidth();
      
      ImGui::End();
    }
    // *************************************************************************
    // 渲染指令
    // 设置背景颜色
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);

    // 清除颜色缓冲和深度缓冲
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ************************************************************************* 
    // 着色器启动
    sceneShader.use();

    // 传入uniform值
    // 时间因子
    factor = glfwGetTime();
    sceneShader.setFloat(locFactor, factor * 0.01f);

    // 相机位置
    sceneShader.setVec3(locViewPos, camera.Position);

    // 材质属性传入着色器中
    sceneShader.setVec3(locMaterialAmbientColor, materialAmbientColor);
    // sceneShader.setInt(locMaterialDiffuseWoodMap, diffuseWoodMap);
    // sceneShader.setInt(locMaterialDiffuseBrickMap, diffuseBrickMap);
    sceneShader.setFloat(locMaterialShininess, shininess);
    sceneShader.setInt("diffuseWoodMap", 0);
    sceneShader.setInt("diffuseBrickMap", 1);
    sceneShader.setInt("diffuseWindowMap", 2);

    // 平行光源属性传入着色器中
    // 设置平行光源位置，沿着x轴往返运动
    glm::vec3 parallelLightPos = glm::vec3(parallelLightPosition.x * glm::sin(glfwGetTime()), parallelLightPosition.y, parallelLightPosition.z);
    sceneShader.setVec3(locDirectionLightDirection, parallelLightPos);
    sceneShader.setVec3(locDirectionLightColor, parallelLightColor);
    sceneShader.setVec3(locDirectionLightAmbientStrength, parallelLightAmbientStrength);
    sceneShader.setVec3(locDirectionLightDiffuseStrength, parallelLightDiffuseStrength);
    sceneShader.setVec3(locDirectionLightSpecularStrength, parallelLightSpecularStrength);

    // 点光源属性传入着色器中
    for (unsigned int i = 0; i < 4; i++)
    {
      // 位置属性传入着色器中
      sceneShader.setVec3(locPointLightPosition[i], pointLightPositions[i]);
      // 颜色属性传入着色器中
      sceneShader.setVec3(locPointLightColor[i], pointLightColors[i]);
      
      // 光照强度属性传入着色器中
      sceneShader.setVec3(locPointLightAmbientStrength[i], lightAmbientStrength);
      sceneShader.setVec3(locPointLightDiffuseStrength[i], lightDiffuseStrength);
      sceneShader.setVec3(locPointLightSpecularStrength[i], lightSpecularStrength);
      
      // 衰减属性传入着色器中
      sceneShader.setFloat(locPointLightAttenuationConstant[i], lightAttenuationConstant);
      sceneShader.setFloat(locPointLightAttenuationLinear[i], lightAttenuationLinear);
      sceneShader.setFloat(locPointLightAttenuationQuadratic[i], lightAttenuationQuadratic);
    }
    
    // ************************************************************

    // ************************************************************
    // 绘制地板
    glBindVertexArray(planeGeometry.VAO);

    sceneShader.setInt(locObjectType, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseWoodMap);

    glm::mat4 modelPlane = glm::mat4(1.0f);
    modelPlane = glm::rotate(modelPlane, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));

    sceneShader.setFloat("uvScale", 0.25f); // 设置纹理缩放，缩小4倍，因为纹理坐标是[0,1]，所以缩小4倍后，纹理坐标为[0, 0.25]。
    sceneShader.setMat4("model", modelPlane);

    glDrawElements(GL_TRIANGLES, planeGeometry.indices.size(), GL_UNSIGNED_INT, 0);

    glBindVertexArray(0); // 解绑VAO

    // ************************************************************
    // 立方体
    // 创建视图矩阵view
    glm::mat4 view = glm::mat4(1.0f);
    view = camera.GetViewMatrix();
    sceneShader.setMat4(locView, view);

    // 创建投影矩阵projection
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(fov), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
    sceneShader.setMat4(locProjection, projection);

    // 设置2个立方体位置
    for(unsigned int i = 0; i < sizeof(cubePositions) / sizeof(cubePositions[0]); i++)
    {
      // 绑定VAO
      glBindVertexArray(boxGeometry.VAO);

      sceneShader.setInt(locObjectType, 1);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, diffuseBrickMap);

      // 创建模型矩阵
      glm::mat4 model = glm::mat4(1.0f);
      // 根据位置数组来平移立方体
      model = glm::translate(model, cubePositions[i]);
      // 根据缩放比例数组来缩放立方体
      model = glm::scale(model, cubeScales[i]);
      // 使用四元数让立方体自旋转
      // glm::qua<float> quat = glm::quat(glm::vec3(factor, factor, factor) * 0.5f);
      // model = model * glm::mat4_cast(quat);
      sceneShader.setMat4(locModel, model);

      // 绘制立方体
      glDrawElements(GL_TRIANGLES, boxGeometry.indices.size(), GL_UNSIGNED_INT, 0);

      glBindVertexArray(0); // 解绑VAO
    }

    // ************************************************************
    // 绘制窗户面板

    // // 绘制窗户面板（直接循环绘制）
    // for (unsigned int i = 0; i < grassPositions.size(); i++)
    // {
    //   glBindVertexArray(windowGeometry.VAO);

    //   sceneShader.setInt(locObjectType, 2);
    //   glActiveTexture(GL_TEXTURE2);
    //   glBindTexture(GL_TEXTURE_2D, diffuseWindowMap);

    //   sceneShader.setFloat("uvScale", 1.0f); 

    //   glm::mat4 model = glm::mat4(1.0f);
    //   model = glm::translate(model, grassPositions[i]);
    //   sceneShader.setMat4("model", model);
    //   glDrawElements(GL_TRIANGLES, windowGeometry.indices.size(), GL_UNSIGNED_INT, 0);
    //   glBindVertexArray(0);
    // }

    // 对透明物体进行动态排序
    std::map<float, glm::vec3> sorted;
    for (unsigned int i = 0; i < grassPositions.size(); i++)
    {
      float distance = glm::length(camera.Position - grassPositions[i]);
      sorted[distance] = grassPositions[i];
    }

    for (std::map<float, glm::vec3>::reverse_iterator iterator = sorted.rbegin(); iterator != sorted.rend(); iterator++)
    {
      glBindVertexArray(windowGeometry.VAO);

      sceneShader.setInt(locObjectType, 2);
      glActiveTexture(GL_TEXTURE2);
      glBindTexture(GL_TEXTURE_2D, diffuseWindowMap);

      sceneShader.setFloat("uvScale", 1.0f); 

      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, iterator->second);
      sceneShader.setMat4("model", model);
      glDrawElements(GL_TRIANGLES, windowGeometry.indices.size(), GL_UNSIGNED_INT, 0);
      glBindVertexArray(0);
    }


    // ************************************************************
    // 绘制灯光球体
    // 使用灯光物体着色器
    lightObjectShader.use();
    lightObjectShader.setMat4(locLightView, view);
    lightObjectShader.setMat4(locLightProjection, projection);

    // 先绑定VAO
    // 再绑定球体
    glBindVertexArray(sphereGeometry.VAO);

    // 绘制平行光源
    // 创建灯光物体模型矩阵
    // 设置平行光源位置
    glm::mat4 parallelLightModel = glm::mat4(1.0f);
    parallelLightModel = glm::translate(parallelLightModel, parallelLightPosition);
    lightObjectShader.setMat4(locLightModel, parallelLightModel);
    lightObjectShader.setVec3("lightColor", lightColor);

    glDrawElements(GL_TRIANGLES, sphereGeometry.indices.size(), GL_UNSIGNED_INT, 0);

    // 绘制点光源
    for (unsigned int i = 0; i < 4; i++)
    {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, pointLightPositions[i]);

      lightObjectShader.setMat4("model", model);
      lightObjectShader.setVec3("lightColor", pointLightColors[i]);

      // 绑定VAO
      glBindVertexArray(sphereGeometry.VAO);
      // 绘制
      glDrawElements(GL_TRIANGLES, sphereGeometry.indices.size(), GL_UNSIGNED_INT, 0);
    }

    // ************************************************************
    // 渲染ImGui部分
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

  planeGeometry.dispose();
  boxGeometry.dispose();
  sphereGeometry.dispose();
  glfwTerminate();
  return 0;
}

// 窗口大小回调
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
  glViewport(0, 0, width, height);
    SCREEN_WIDTH = width;
    SCREEN_HEIGHT = height;
}

// 键盘输入处理
void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
  {
        std::cout<< "Esc is pressed" << std::endl;
    glfwSetWindowShouldClose(window, true);
  }

  // 相机按键控制
    float cameraSpeed = 2.5f * deltaTime;
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    camera.Position = cameraPos;
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

  camera.ProcessRotationByOffset(xoffset, yoffset);
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

        glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));
        glm::vec3 up = cameraUp;

        cameraPos -= right * xoffset * sensitivity;
        cameraPos += up * yoffset * sensitivity;
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
unsigned int loadTexture(const char* path, int& outWidth, int& outHeight, int& outChannels, GLint wrapS, GLint wrapT) {
    unsigned int textureID = 0;

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