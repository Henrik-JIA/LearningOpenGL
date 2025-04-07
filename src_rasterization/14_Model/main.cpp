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
  outTexCoord = TexCoords;

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
// 动态时间值（变化值）
uniform float factor;
// 笑脸贴图
uniform sampler2D awesomeMap;

// 定向光计算
vec3 CalcDirectionLight(DirectionLight light, vec3 normal, vec3 viewDir);
// 点光源计算
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
// 聚光灯计算
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

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

  // 聚光光源
  result += CalcSpotLight(spotLight, normal, outFragPos, viewDir) 
            * vec3(texture(awesomeMap, outTexCoord));

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
  FragColor = vec4(lightColor, 1.0f);
}
)";

// 函数声明
void framebuffer_size_callback(GLFWwindow* window, int width, int height); // 窗口大小回调函数
void processInput(GLFWwindow *window); // 处理输入
void mouse_callback(GLFWwindow *window, double xpos, double ypos); // 鼠标回调函数
void mouse_button_calback(GLFWwindow *window, int button, int action, int mods); // 鼠标按钮回调函数
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset); // 滚轮回调函数
unsigned int loadTexture(const char* path, int& outWidth, int& outHeight, int& outChannels); // 加载纹理

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
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f)); // 相机类
glm::vec3 cameraPos = camera.Position; // 相机位置
glm::vec3 cameraFront = camera.Front; // 相机前向
glm::vec3 cameraUp = camera.Up; // 相机上向

// 材质属性
glm::vec3 materialAmbientColor = glm::vec3(1.0f, 0.5f, 0.31f);
int materialDiffuseMap = 0; // 漫反射颜色(纹理)
int materialSpecularMap = 1; // 高光颜色(纹理)
int materialAwesomeMap = 2; // 聚光灯颜色(纹理)
int shininess = 128; // 高光指数

// 光源属性（初始值）
// glm::vec3 lightPosition = glm::vec3(0.0, 0.0, -2.0); // 光照位置
glm::vec3 lightPosition = glm::vec3(1.2f, 1.0f, 2.0f); // 光照位置
glm::vec3 lightDirection = glm::vec3(0.0, 0.0, -1.0); // 光照方向
glm::vec3 parallelLightDirection = glm::vec3(0.0, 0.0, -1.0); // 平行光方向
// glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f); // 光照颜色
glm::vec3 lightColor = glm::vec3(1.0f, 0.5f, 0.0f); // 光照颜色橘黄色
float cutOff = glm::cos(glm::radians(12.5f)); // 聚光灯的切光角，cos值
float outerCutOff = glm::cos(glm::radians(17.5f)); // 聚光灯的外切光角，cos值
glm::vec3 lightAmbientStrength = glm::vec3(0.05f, 0.05f, 0.05f); // 环境光强度
glm::vec3 lightDiffuseStrength = glm::vec3(0.8f, 0.8f, 0.8f); // 漫反射强度
glm::vec3 lightSpecularStrength = glm::vec3(1.0f, 1.0f, 1.0f); // 镜面反射强度
glm::vec3 spotLightAmbientStrength = glm::vec3(0.0f, 0.0f, 0.0f); // 聚光灯环境光强度
glm::vec3 spotLightDiffuseStrength = glm::vec3(1.0f, 1.0f, 1.0f); // 聚光灯漫反射强度
glm::vec3 spotLightSpecularStrength = glm::vec3(1.0f, 1.0f, 1.0f); // 聚光灯镜面反射强度
float lightAttenuationConstant = 1.0f; // 衰减常数项，常数项一般都是1.0
float lightAttenuationLinear = 0.09f; // 衰减一次项，对应50m距离
float lightAttenuationQuadratic = 0.032f; // 衰减二次项，对应50m距离

// 创建多个立方体初始位置
glm::vec3 cubePositions[] = {
    glm::vec3( 0.0f,  0.0f,  0.0f), 
    glm::vec3( 2.0f,  5.0f, -15.0f), 
    glm::vec3(-1.5f, -2.2f, -2.5f),  
    glm::vec3(-3.8f, -2.0f, -12.3f),  
    glm::vec3( 2.4f, -0.4f, -3.5f),  
    glm::vec3(-1.7f,  3.0f, -7.5f),  
    glm::vec3( 1.3f, -2.0f, -2.5f),  
    glm::vec3( 1.5f,  2.0f, -2.5f), 
    glm::vec3( 1.5f,  0.2f, -1.5f), 
    glm::vec3(-1.3f,  1.0f, -1.5f)  
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

using namespace std;

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
  // // 启用混合
  // glEnable(GL_BLEND);
  // // 设置混合函数
  // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  // 设置绘制模式
  // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_DEPTH_TEST); // 开启深度测试

  // 创建着色器（包括顶点着色器、片段着色器、着色器程序、uniform设置）
  // 物体着色器
  Shader ourShader = Shader::FromSource(vertexShaderSource, fragmentShaderSource);
  // 灯光物体着色器
  Shader lightObjectShader = Shader::FromSource(light_object_vert, light_object_frag);

  // 获取uniform变量location
  // 物体着色器：
  // 时间因子
  GLint locFactor = ourShader.getUniformLocation("factor");

  // MVP矩阵
  GLint locModel = ourShader.getUniformLocation("model");
  GLint locView = ourShader.getUniformLocation("view");
  GLint locProjection = ourShader.getUniformLocation("projection");
  
  // 相机位置
  GLint locViewPos = ourShader.getUniformLocation("viewPos");
  
  // 材质纹理属性位置
  GLint locMaterialAmbientColor = ourShader.getUniformLocation("material.ambientColor");
  GLint locMaterialDiffuseMap = ourShader.getUniformLocation("material.diffuseMap");
  GLint locMaterialSpecularMap = ourShader.getUniformLocation("material.specularMap");
  GLint locAwesomeMap = ourShader.getUniformLocation("awesomeMap");
  GLint locMaterialShininess = ourShader.getUniformLocation("material.shininess");
  
  // 平行光源属性位置
  GLint locDirectionLightColor = ourShader.getUniformLocation("directionLight.color");
  GLint locDirectionLightDirection = ourShader.getUniformLocation("directionLight.parallelLightDirection");
  GLint locDirectionLightAmbientStrength = ourShader.getUniformLocation("directionLight.ambientStrength");
  GLint locDirectionLightDiffuseStrength = ourShader.getUniformLocation("directionLight.diffuseStrength");
  GLint locDirectionLightSpecularStrength = ourShader.getUniformLocation("directionLight.specularStrength");

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
    locPointLightPosition.push_back(ourShader.getUniformLocation("pointLights[" + to_string(i) + "].position"));
    locPointLightColor.push_back(ourShader.getUniformLocation("pointLights[" + to_string(i) + "].color"));
    locPointLightAmbientStrength.push_back(ourShader.getUniformLocation("pointLights[" + to_string(i) + "].ambientStrength"));
    locPointLightDiffuseStrength.push_back(ourShader.getUniformLocation("pointLights[" + to_string(i) + "].diffuseStrength"));
    locPointLightSpecularStrength.push_back(ourShader.getUniformLocation("pointLights[" + to_string(i) + "].specularStrength"));
    locPointLightAttenuationConstant.push_back(ourShader.getUniformLocation("pointLights[" + to_string(i) + "].attenuationConstant"));
    locPointLightAttenuationLinear.push_back(ourShader.getUniformLocation("pointLights[" + to_string(i) + "].attenuationLinear"));
    locPointLightAttenuationQuadratic.push_back(ourShader.getUniformLocation("pointLights[" + to_string(i) + "].attenuationQuadratic")); 
  }

  // 聚光灯属性位置
  GLint locSpotLightPosition = ourShader.getUniformLocation("spotLight.position");
  GLint locSpotLightDirection = ourShader.getUniformLocation("spotLight.direction");
  GLint locSpotLightColor = ourShader.getUniformLocation("spotLight.color");
  GLint locSpotLightCutOff = ourShader.getUniformLocation("spotLight.cutOff");
  GLint locSpotLightOuterCutOff = ourShader.getUniformLocation("spotLight.outerCutOff");
  GLint locSpotLightAmbientStrength = ourShader.getUniformLocation("spotLight.ambientStrength");
  GLint locSpotLightDiffuseStrength = ourShader.getUniformLocation("spotLight.diffuseStrength");
  GLint locSpotLightSpecularStrength = ourShader.getUniformLocation("spotLight.specularStrength");
  GLint locSpotLightAttenuationConstant = ourShader.getUniformLocation("spotLight.attenuationConstant");
  GLint locSpotLightAttenuationLinear = ourShader.getUniformLocation("spotLight.attenuationLinear");
  GLint locSpotLightAttenuationQuadratic = ourShader.getUniformLocation("spotLight.attenuationQuadratic");
  

  // 灯光物体着色器：
  // MVP矩阵位置
  GLint locLightModel = lightObjectShader.getUniformLocation("model");
  GLint locLightView = lightObjectShader.getUniformLocation("view");
  GLint locLightProjection = lightObjectShader.getUniformLocation("projection");

  // 重建几何体
  PlaneGeometry planeGeometry(1.0f, 1.0f, 1.0f, 1.0f); // 创建平面几何体（包括几何的顶点、index、VAO、VBO、EBO）
  BoxGeometry boxGeometry(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f); // 创建立方体几何体
  SphereGeometry sphereGeometry(0.1, 10.0, 10.0); // 创建球体 

  // 纹理加载
  unsigned int diffuseMap, specularMap, awesomeMap;
  int width1, height1, channels1;
  int width2, height2, channels2;
  int width3, height3, channels3;
  // 纹理1
  diffuseMap = loadTexture("../static/texture/container2.png", width1, height1, channels1);
  // 设置特殊参数（仅对texture1）
  glBindTexture(GL_TEXTURE_2D, diffuseMap);
  float borderColor[] = {0.3f, 0.1f, 0.7f, 1.0f};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
  // 纹理2
  specularMap = loadTexture("../static/texture/container2_specular.png", width2, height2, channels2);
  // 纹理3
  awesomeMap = loadTexture("../static/texture/awesomeface.png", width3, height3, channels3);

  // 加载第一个模型（nanosuit）
  Model ourModel("../static/model/nanosuit/nanosuit.obj");

  // 加载第二个模型（backpack加载时间）
  Model ourModel2("../static/model/backpack/backpack.obj");

  // 渲染循环
  while (!glfwWindowShouldClose(window))
  {
    // 计算时间差，用于计算帧率以限制相机因时间变化而移动过快
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    // 处理输入（使用最新的deltaTime）
    processInput(window);

    // 在标题中显示帧率信息
    int fps_value = (int)round(ImGui::GetIO().Framerate);
    int ms_value = (int)round(1000.0f / ImGui::GetIO().Framerate);
    std::string FPS = std::to_string(fps_value);
    std::string ms = std::to_string(ms_value);
    std::string newTitle = "LearnOpenGL - " + ms + " ms/frame " + FPS;
    glfwSetWindowTitle(window, newTitle.c_str());

    // 开始ImGui框架
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

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

    // 渲染指令
    // 设置背景颜色
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);

    // 清除颜色缓冲和深度缓冲
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ourShader.use();    
    factor = glfwGetTime();
    ourShader.setFloat(locFactor, factor * 0.01f);

    ourShader.setVec3(locViewPos, camera.Position);

    // 材质属性传入着色器中
    ourShader.setVec3(locMaterialAmbientColor, materialAmbientColor);
    ourShader.setInt(locMaterialDiffuseMap, materialDiffuseMap);
    ourShader.setInt(locMaterialSpecularMap, materialSpecularMap); 
    ourShader.setInt(locAwesomeMap, materialAwesomeMap);
    ourShader.setFloat(locMaterialShininess, shininess);

    // 平行光源属性传入着色器中
    // 设置平行光源位置，沿着x轴往返运动
    glm::vec3 lightPos = glm::vec3(lightPosition.x * glm::sin(glfwGetTime()), lightPosition.y, lightPosition.z);
    ourShader.setVec3(locDirectionLightDirection, lightPos);
    ourShader.setVec3(locDirectionLightColor, lightColor);
    ourShader.setVec3(locDirectionLightAmbientStrength, lightAmbientStrength);
    ourShader.setVec3(locDirectionLightDiffuseStrength, lightDiffuseStrength);
    ourShader.setVec3(locDirectionLightSpecularStrength, lightSpecularStrength);

    // 点光源属性传入着色器中
    for (unsigned int i = 0; i < 4; i++)
    {
      ourShader.setVec3(locPointLightPosition[i], pointLightPositions[i]);
      ourShader.setVec3(locPointLightColor[i], pointLightColors[i]);
      ourShader.setFloat(locPointLightAttenuationConstant[i], lightAttenuationConstant);
      ourShader.setFloat(locPointLightAttenuationLinear[i], lightAttenuationLinear);
      ourShader.setFloat(locPointLightAttenuationQuadratic[i], lightAttenuationQuadratic);
      ourShader.setVec3(locPointLightAmbientStrength[i], lightAmbientStrength);
      ourShader.setVec3(locPointLightDiffuseStrength[i], lightDiffuseStrength);
      ourShader.setVec3(locPointLightSpecularStrength[i], lightSpecularStrength);
    }
    
    // 聚光灯属性传入着色器中
    ourShader.setVec3(locSpotLightPosition, cameraPos);
    ourShader.setVec3(locSpotLightDirection, cameraFront);
    ourShader.setVec3(locSpotLightColor, lightColor);
    ourShader.setFloat(locSpotLightCutOff, cutOff);
    ourShader.setFloat(locSpotLightOuterCutOff, outerCutOff);
    ourShader.setVec3(locSpotLightAmbientStrength, spotLightAmbientStrength);
    ourShader.setVec3(locSpotLightDiffuseStrength, spotLightDiffuseStrength);
    ourShader.setVec3(locSpotLightSpecularStrength, spotLightSpecularStrength);
    ourShader.setFloat(locSpotLightAttenuationConstant, lightAttenuationConstant);
    ourShader.setFloat(locSpotLightAttenuationLinear, lightAttenuationLinear);
    ourShader.setFloat(locSpotLightAttenuationQuadratic, lightAttenuationQuadratic);
   
    // 激活纹理单元，并绑定纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseMap);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, specularMap);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, awesomeMap);

    // 绑定VAO
    // 先绑定立方体
    glBindVertexArray(boxGeometry.VAO);

    // 创建视图矩阵view
    glm::mat4 view = glm::mat4(1.0f);
    view = camera.GetViewMatrix();
    ourShader.setMat4(locView, view);

    // 创建投影矩阵projection
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(fov), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
    ourShader.setMat4(locProjection, projection);

    // 设置10个立方体位置
    for(unsigned int i = 0; i < sizeof(cubePositions) / sizeof(cubePositions[0]); i++)
    {
      // 创建模型矩阵
      glm::mat4 model = glm::mat4(1.0f);
      // 根据位置数组来平移立方体
      model = glm::translate(model, cubePositions[i]);
      // 使用四元数让立方体自旋转
      // glm::qua<float> quat = glm::quat(glm::vec3(factor, factor, factor) * 0.5f);
      // model = model * glm::mat4_cast(quat);
      ourShader.setMat4(locModel, model);

      // 绘制立方体
      glDrawElements(GL_TRIANGLES, boxGeometry.indices.size(), GL_UNSIGNED_INT, 0);
    }

    // 绘制第一个模型（nanosuit）
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.2f, -1.3f, 2.5f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(0.15f));
    ourShader.setMat4(locModel, modelMatrix);
    ourModel.Draw(ourShader);

    // 绘制第二个模型（backpack）
    glm::mat4 modelMatrix2 = glm::mat4(1.0f);
    modelMatrix2 = glm::translate(modelMatrix2, glm::vec3(1.2f, -1.3f, 2.5f));
    modelMatrix2 = glm::scale(modelMatrix2, glm::vec3(0.5f));
    ourShader.setMat4(locModel, modelMatrix2);
    ourModel2.Draw(ourShader);

    // 绘制灯光物体
    // 使用灯光物体着色器
    lightObjectShader.use(); 
    lightObjectShader.setMat4(locLightView, view);
    lightObjectShader.setMat4(locLightProjection, projection);

    // 先绑定VAO
    // 再绑定球体
    glBindVertexArray(sphereGeometry.VAO);

    // 绘制平行光源
    // 创建灯光物体模型矩阵
    glm::mat4 lightModel = glm::mat4(1.0f);
    // 设置平行光源位置
    lightModel = glm::translate(lightModel, lightPos);
    lightObjectShader.setMat4(locLightModel, lightModel);
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

    // 绘制聚光灯
    // 创建聚光灯模型矩阵
    glm::mat4 spotLightModel = glm::mat4(1.0f);
    // 设置聚光灯位置
    spotLightModel = glm::translate(spotLightModel, cameraPos);
    lightObjectShader.setMat4("model", spotLightModel);
    lightObjectShader.setVec3("lightColor", lightColor);
    // 绑定VAO
    glBindVertexArray(sphereGeometry.VAO);
    // 绘制
    glDrawElements(GL_TRIANGLES, sphereGeometry.indices.size(), GL_UNSIGNED_INT, 0);

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

  // planeGeometry.dispose();
  boxGeometry.dispose();
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

unsigned int loadTexture(const char* path, int& outWidth, int& outHeight, int& outChannels) {
    unsigned int textureID = 0;
    outWidth = outHeight = outChannels = 0; // 初始化输出参数

    // 加载awesomeMap时临时翻转，添加垂直翻转操作
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &outWidth, &outHeight, &outChannels, 0);
    if (data) {
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        GLenum format = GL_RGB;
        if (outChannels == 1)       format = GL_RED;
        else if (outChannels == 3)  format = GL_RGB;
        else if (outChannels == 4)  format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, outWidth, outHeight, 0, 
                    format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cerr << "Texture failed to load: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID; // 只返回textureID
}