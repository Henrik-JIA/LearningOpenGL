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

#include <tool/gui.h>

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

Camera camera(SCREEN_WIDTH, SCREEN_HEIGHT, glm::vec3(0.0, 1.0, 10.0));

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
  
  // 物体着色器，仅绘制纯色物体，不涉及光照
  Shader lightObjectShader = Shader::FromSource(light_sphere_vert, light_sphere_frag);
  // 灯光物体着色器，光照对物体的影像
  Shader lightSourceShader = Shader::FromSource(light_source_vert, light_source_frag);
  // 天空盒子着色器
  Shader skyboxShader = Shader::FromSource(cube_map_vert, cube_map_frag);
  // 帧缓冲着色器
  Shader frameBufferShader = Shader::FromSource(frame_buffer_quad_vert, frame_buffer_quad_frag);
  // 反射着色器
  Shader reflectShader = Shader::FromSource(reflect_object_vert, reflect_object_frag);
  // 折射着色器
  Shader refractShader = Shader::FromSource(refract_object_vert, refract_object_frag);
  
  // Shader sceneShader = Shader::FromFile("../src/20_Cubemaps_02/shader/scene_vert.glsl", "../src/20_Cubemaps_02/shader/scene_frag.glsl");
  // Shader lightSourceShader = Shader::FromFile("../src/20_Cubemaps_02/shader/light_vert.glsl", "../src/20_Cubemaps_02/shader/light_frag.glsl");
  // Shader frameBufferShader = Shader::FromFile("../src/20_Cubemaps_02/shader/frame_buffer_quad_vert.glsl", "../src/20_Cubemaps_02/shader/frame_buffer_quad_frag.glsl");
  // Shader skyboxShader = Shader::FromFile("../src/20_Cubemaps_02/shader/cube_map_vert.glsl", "../src/20_Cubemaps_02/shader/cube_map_frag.glsl");
  // Shader reflectShader = Shader::FromFile("../src/20_Cubemaps_02/shader/reflect_object_vert.glsl", "../src/20_Cubemaps_02/shader/reflect_object_frag.glsl");
  // Shader refractShader = Shader::FromFile("../src/20_Cubemaps_02/shader/refract_object_vert.glsl", "../src/20_Cubemaps_02/shader/refract_object_frag.glsl");


  float factor = 0.0;

  float fov = 45.0f; // 视锥体的角度
  
  int NR_POINT_LIGHTS = 4;

  ImVec4 clear_color = ImVec4(25.0 / 255.0, 25.0 / 255.0, 25.0 / 255.0, 0.1); // 25, 25, 25

  glm::vec3 lightPosition = glm::vec3(1.0, 2.5, 2.0); // 光照位置

  // 重建几何体
  PlaneGeometry groundGeometry(10.0, 10.0);            // 地面
  PlaneGeometry grassGeometry(1.0, 1.0);               // 草丛
  PlaneGeometry frameGeometry(2.0, 2.0);               // 窗口平面
  BoxGeometry containerGeometry(1.0, 1.0, 1.0);              // 箱子
  BoxGeometry skyboxGeometry(1.0, 1.0, 1.0);           // 天空盒
  SphereGeometry pointLightGeometry(0.04, 10.0, 10.0); // 点光源位置显示

  // 纹理加载
  unsigned int woodMap = loadTexture("../static/texture/wood.png", GL_REPEAT, GL_REPEAT);                         // 地面
  unsigned int brickMap = loadTexture("../static/texture/brick_diffuse.jpg", GL_REPEAT, GL_REPEAT);               // 砖块
  unsigned int grassMap = loadTexture("../static/texture/blending_transparent_window.png", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE); // 草丛

  // 点光源的位置
  glm::vec3 pointLightPositions[] = {
      glm::vec3(3.0f, 1.5f, 3.0f),    // 右前方
      glm::vec3(-3.0f, 2.5f, -3.0f),  // 左后方
      glm::vec3(-5.0f, 1.8f, 2.0f),   // 左侧更远处
      glm::vec3(4.0f, 2.2f, -2.0f)};  // 右后方
      // glm::vec3(0.7f, 1.0f, 1.5f),
      // glm::vec3(2.3f, 3.0f, -4.0f),
      // glm::vec3(-4.0f, 2.0f, 1.0f),
      // glm::vec3(1.4f, 2.0f, 1.3f)};
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
  // 天空盒贴图
  // vector<string> faces{
  //     "../static/texture/Park3Med/px.jpg",
  //     "../static/texture/Park3Med/nx.jpg",
  //     "../static/texture/Park3Med/py.jpg",
  //     "../static/texture/Park3Med/ny.jpg",
  //     "../static/texture/Park3Med/pz.jpg",
  //     "../static/texture/Park3Med/nz.jpg"};

  // vector<string> faces{
  //     "../static/texture/Mountains_Lake/px.jpg", // +X 右
  //     "../static/texture/Mountains_Lake/nx.jpg", // -X 左
  //     "../static/texture/Mountains_Lake/py.jpg", // +Y 上
  //     "../static/texture/Mountains_Lake/ny.jpg", // -Y 下
  //     "../static/texture/Mountains_Lake/pz.jpg", // +Z 前
  //     "../static/texture/Mountains_Lake/nz.jpg", // -Z 后
  //     };
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
    
    // 这些光照属性设置必须在渲染循环内，因为每帧都会更新。
    // 设置点光源光照属性
    float radius = 5.0f;
    float camX = sin(glfwGetTime() * 0.5) * radius;
    float camZ = cos(glfwGetTime() * 0.5) * radius;
    pointLightPositions[0].z = camZ;
    pointLightPositions[0].x = camX;
    for (unsigned int i = 0; i < NR_POINT_LIGHTS; i++)
    {
      lightSourceShader.setVec3("pointLights[" + std::to_string(i) + "].position", pointLightPositions[i]);
      lightSourceShader.setVec3("pointLights[" + std::to_string(i) + "].ambient", 0.01f, 0.01f, 0.01f);
      lightSourceShader.setVec3("pointLights[" + std::to_string(i) + "].diffuse", pointLightColors[i]);
      lightSourceShader.setVec3("pointLights[" + std::to_string(i) + "].specular", 1.0f, 1.0f, 1.0f);
      lightSourceShader.setFloat("pointLights[" + std::to_string(i) + "].constant", 1.0f);
      lightSourceShader.setFloat("pointLights[" + std::to_string(i) + "].linear", 0.09f);
      lightSourceShader.setFloat("pointLights[" + std::to_string(i) + "].quadratic", 0.032f);
    }

    // 设置平行光光照属性
    // glm::vec3 lightPos = glm::vec3(lightPosition.x * glm::sin(glfwGetTime()) * 2.0, lightPosition.y, lightPosition.z); // 动态位置
    glm::vec3 lightPos = glm::vec3(lightPosition.x, lightPosition.y, lightPosition.z); // 固定位置
    lightSourceShader.setVec3("directionLight.direction", lightPos); // 光源位置
    // 设置平行光光照属性
    lightSourceShader.setVec3("directionLight.ambient", 0.01f, 0.01f, 0.01f);
    lightSourceShader.setVec3("directionLight.diffuse", 0.9f, 0.9f, 0.9f); // 将光照调暗了一些以搭配场景
    lightSourceShader.setVec3("directionLight.specular", 1.0f, 1.0f, 1.0f);


    // 绘制地板
    // ********************************************************
    
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, woodMap);

    // // 旋转地板
    // model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
    // // 设置地板的UV缩放
    // lightSourceShader.setFloat("uvScale", 4.0f);
    // lightSourceShader.setMat4("model", model);

    // glBindVertexArray(groundGeometry.VAO); // 绑定VAO

    // glDrawElements(GL_TRIANGLES, groundGeometry.indices.size(), GL_UNSIGNED_INT, 0);
    
    // glBindVertexArray(0); // 解绑VAO
    
    // ********************************************************

    // 绘制砖块
    // ----------------------------------------------------------
    glBindVertexArray(containerGeometry.VAO); // 绑定砖块的VAO
    glBindTexture(GL_TEXTURE_2D, brickMap); // 绑定砖块的纹理
    
    // 第一个砖块（左侧）反射
    reflectShader.use(); // 激活反射着色器
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-3.0f, 2.0f, 0.0f)); // X轴-3位置
    model = glm::rotate(model, glm::radians((float)glfwGetTime() * 20.0f), glm::vec3(1.0, 0.0, 0.0));
    model = glm::scale(model, glm::vec3(2.0, 2.0, 2.0));    
    reflectShader.setMat4("model", model);
    glDrawElements(GL_TRIANGLES, containerGeometry.indices.size(), GL_UNSIGNED_INT, 0);

    // 第二个砖块（中间）漫反射
    lightSourceShader.use(); // 激活漫反射着色器
    lightSourceShader.setFloat("uvScale", 1.0f);
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 2.0f, 0.0f));  // X轴0位置
    model = glm::rotate(model, glm::radians((float)glfwGetTime() * 20.0f), glm::vec3(1.0, 0.0, 0.0));
    model = glm::scale(model, glm::vec3(2.0, 2.0, 2.0));   
    lightSourceShader.setMat4("model", model);
    glDrawElements(GL_TRIANGLES, containerGeometry.indices.size(), GL_UNSIGNED_INT, 0);

    // 第三个砖块（右侧）折射
    refractShader.use(); // 激活折射着色器
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(3.0f, 2.0f, 0.0f));  // X轴+3位置
    model = glm::rotate(model, glm::radians((float)glfwGetTime() * 20.0f), glm::vec3(1.0, 0.0, 0.0));
    model = glm::scale(model, glm::vec3(2.0f));
    refractShader.setMat4("model", model);
    glDrawElements(GL_TRIANGLES, containerGeometry.indices.size(), GL_UNSIGNED_INT, 0);
    
    glBindVertexArray(0); // 解绑VAO
    // ----------------------------------------------------------

    // 绘制草丛或透明窗户面板
    // ----------------------------------------------------------
    // glBindVertexArray(grassGeometry.VAO); // 绑定VAO
    // glBindTexture(GL_TEXTURE_2D, grassMap);

    // // 激活漫反射着色器
    // lightSourceShader.use();

    // // 对透明物体进行动态排序
    // std::map<float, glm::vec3> sorted;
    // for (unsigned int i = 0; i < grassPositions.size(); i++)
    // {
    //   float distance = glm::length(camera.Position - grassPositions[i]);
    //   sorted[distance] = grassPositions[i];
    // }

    // for (std::map<float, glm::vec3>::reverse_iterator iterator = sorted.rbegin(); iterator != sorted.rend(); iterator++)
    // {
    //   model = glm::mat4(1.0f);
    //   model = glm::translate(model, iterator->second);
    //   lightSourceShader.setMat4("model", model);
    //   glDrawElements(GL_TRIANGLES, grassGeometry.indices.size(), GL_UNSIGNED_INT, 0);
    // }

    // glBindVertexArray(0); // 解绑VAO
    // ----------------------------------------------------------

    // 绘制灯光球体（没有使用贴图，使用灯光颜色为球体颜色）
    // ************************************************************
    // lightObjectShader.use();
    // // 绘制平行光位置球体
    // lightObjectShader.setMat4("view", view);
    // lightObjectShader.setMat4("projection", projection);

    // model = glm::mat4(1.0f);
    // model = glm::translate(model, lightPos); // 这里lightPos按照x轴往复运动

    // lightObjectShader.setMat4("model", model);
    // lightObjectShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));

    // glBindVertexArray(pointLightGeometry.VAO); // 绑定VAO

    // glDrawElements(GL_TRIANGLES, pointLightGeometry.indices.size(), GL_UNSIGNED_INT, 0);

    // // 绘制点光源位置球体
    // for (unsigned int i = 0; i < 4; i++)
    // {
    //   model = glm::mat4(1.0f);
    //   model = glm::translate(model, pointLightPositions[i]);

    //   lightObjectShader.setMat4("model", model);
    //   lightObjectShader.setVec3("lightColor", pointLightColors[i]);

    //   glDrawElements(GL_TRIANGLES, pointLightGeometry.indices.size(), GL_UNSIGNED_INT, 0);
    // }

    // glBindVertexArray(0); // 解绑VAO

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

  
  grassGeometry.dispose();
  frameGeometry.dispose();
  containerGeometry.dispose();
  skyboxGeometry.dispose();
  groundGeometry.dispose();
  pointLightGeometry.dispose();
  glfwTerminate();

  return 0;
}

// 窗口变动监听
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
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
    glm::mat4 projection = glm::perspective(glm::radians(camera.fov), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);

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