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

// 输出变量
out vec4 FragColor;

// 输入变量
in vec2 outTexCoord;
in vec3 outNormal;
in vec3 outFragPos;

// 相机位置
uniform vec3 viewPos;

// 定义材质结构体，材质在各光照条件下的颜色情况
struct Material{
  vec3 ambientColor; // 环境光颜色
  sampler2D diffuseColor; // 漫反射颜色(纹理)
  sampler2D specularColor; // 高光颜色(纹理)
  float shininess; // 高光指数
};
uniform Material material;

// 定义光结构体，光源属性
struct Light {
    vec3 position; // 光源位置
    vec3 direction; // 光源方向
    vec3 color; // 光源颜色
    
    // 平行光属性
    vec3 parallelLightDirection; // 平行光方向（也就是光照方向，只是不需要计算每一个着色点指向光源的向量了，直接使用光照方向，因为平行光的光源方向是固定的，所有着色点都使用同一个光照方向）

    // 聚光灯属性
    float cutOff; // 聚光灯的切光角
    float outerCutOff; // 聚光灯的外切光角    

    // 光强度属性
    vec3 ambientStrength; // 环境光强度
    vec3 diffuseStrength; // 漫反射强度
    vec3 specularStrength; // 镜面反射强度

    // 点光源衰减项参数
    float attenuationConstant; // 衰减常数项
    float attenuationLinear; // 衰减一次项
    float attenuationQuadratic; // 衰减二次项
};
uniform Light light;

void main() {
  // 物体颜色
  vec4 objectColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);

  // 着色点到光源方向
  vec3 lightDir = normalize(light.position - outFragPos);

  // 光源距离
  float distanceToLight = length(light.position - outFragPos);

  // 漫反射纹理采样颜色
  vec3 diffuseTextureColor = vec3(texture(material.diffuseColor, outTexCoord));

  // 高光纹理采样颜色
  vec3 specularTextureColor = vec3(texture(material.specularColor, outTexCoord));

  // 聚光灯
  vec3 spotLightDirection = normalize(-light.direction); // 光源方向
  float theta = dot(lightDir, spotLightDirection); // 是cos值，着色点到光源的向量与光源方向的夹角余弦值
  float Phi = cos(light.cutOff); // 是cos值，切光角cos值
  bool isInnerLight = theta > Phi; // cos值越大，角度越小。theta > Phi 说明在聚光灯的内部

  // 环境光项
  vec3 ambient = light.ambientStrength * material.ambientColor * diffuseTextureColor;

  // 漫反射项
  vec3 norm = normalize(outNormal);
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = light.diffuseStrength * diff * diffuseTextureColor; 

  // 镜面光项
  // 计算视线方向（这里所有射线均是向外的）
  vec3 viewDir = normalize(viewPos - outFragPos);
  // 计算反射方向
  vec3 reflectDir = reflect(-lightDir, norm);
  // 计算镜面光强度（幂次项shininess越大，高光范围约集中）
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
  vec3 specular = light.specularStrength * spec * specularTextureColor;

  // 衰减项
  float attenuation = 1.0 / (light.attenuationConstant + light.attenuationLinear * distanceToLight + 
                  light.attenuationQuadratic * (distanceToLight * distanceToLight));
  ambient *= attenuation;
  diffuse *= attenuation;
  specular *= attenuation;

  // 提取声明result
  vec3 result;

  // 仅环境光项
  // vec3 result = ambient * vec3(objectColor); 

  // 仅漫反射项
  // vec3 result = diffuse * vec3(objectColor);

  // 仅镜面光项
  // vec3 result = specular * vec3(objectColor);

  // 环境光+漫反射+镜面光
  // vec3 result = (ambient + diffuse + specular) * vec3(objectColor) * light.color;
  
  // 聚光灯
  if (isInnerLight) {
    // 执行光照计算
    result = (ambient + diffuse + specular) * vec3(objectColor) * light.color;
  } else {
    // 否则，使用环境光，让场景在聚光之外时不至于完全黑暗
    result = ambient * vec3(objectColor) * light.color;
  }

  // 最终颜色
  FragColor = vec4(result, 1.0);
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

void main() {
  FragColor = vec4(1);
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
// glm::vec3 materialAmbientColor = glm::vec3(1.0f, 1.0f, 1.0f); // 环境光颜色
glm::vec3 materialAmbientColor = glm::vec3(1.0f, 0.5f, 0.31f);
int materialDiffuseColor = 0; // 漫反射颜色(纹理)
int materialSpecularColor = 1; // 高光颜色(纹理)
int shininess = 128; // 高光指数

// 光源属性（初始值）
glm::vec3 lightPosition = glm::vec3(0.0, 0.0, -2.0); // 光照位置
glm::vec3 lightDirection = glm::vec3(0.0, 0.0, -1.0); // 光照方向
glm::vec3 parallelLightDirection = glm::vec3(0.0, 0.0, -1.0); // 平行光方向
glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f); // 光照颜色
float cutOff = glm::radians(12.5f); // 聚光灯的切光角
float outerCutOff = glm::radians(17.5f); // 聚光灯的外切光角
glm::vec3 lightAmbientStrength = glm::vec3(0.1f, 0.1f, 0.1f); // 环境光强度
glm::vec3 lightDiffuseStrength = glm::vec3(0.9f, 0.9f, 0.9f); // 漫反射强度
glm::vec3 lightSpecularStrength = glm::vec3(1.0f, 1.0f, 1.0f); // 镜面反射强度
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
  Shader lightObjectShader = Shader::FromSource(light_object_vert, light_object_frag);;

  // 获取uniform变量location
  // 物体着色器：
  // 时间因子
  GLint locFactor = ourShader.getUniformLocation("factor");
  // // 纹理参数
  // GLint texture1Location = ourShader.getUniformLocation("texture1");
  // GLint texture2Location = ourShader.getUniformLocation("texture2");
  // MVP矩阵
  GLint locModel = ourShader.getUniformLocation("model");
  GLint locView = ourShader.getUniformLocation("view");
  GLint locProjection = ourShader.getUniformLocation("projection");
  // 相机位置
  GLint locViewPos = ourShader.getUniformLocation("viewPos");
  // 材质属性位置
  GLint locMaterialAmbientColor = ourShader.getUniformLocation("material.ambientColor");
  GLint locMaterialDiffuseColor = ourShader.getUniformLocation("material.diffuseColor");
  GLint locMaterialSpecularColor = ourShader.getUniformLocation("material.specularColor");
  GLint locMaterialShininess = ourShader.getUniformLocation("material.shininess");
  // 光源属性位置
  GLint locLightPos = ourShader.getUniformLocation("light.position");
  GLint locLightDirection = ourShader.getUniformLocation("light.direction");
  GLint locLightColor = ourShader.getUniformLocation("light.color");
  GLint locParallelLightDirection = ourShader.getUniformLocation("light.parallelLightDirection");
  GLint locCutOff = ourShader.getUniformLocation("light.cutOff");
  GLint locOuterCutOff = ourShader.getUniformLocation("light.outerCutOff");
  GLint locLightAmbientStrength = ourShader.getUniformLocation("light.ambientStrength");
  GLint locLightDiffuseStrength = ourShader.getUniformLocation("light.diffuseStrength");
  GLint locLightSpecularStrength = ourShader.getUniformLocation("light.specularStrength");
  GLint locLightAttenuationConstant = ourShader.getUniformLocation("light.attenuationConstant");
  GLint locLightAttenuationLinear = ourShader.getUniformLocation("light.attenuationLinear");
  GLint locLightAttenuationQuadratic = ourShader.getUniformLocation("light.attenuationQuadratic");

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
  unsigned int diffuseMap, specularMap;
  int width1, height1, channels1;
  int width2, height2, channels2;
  // 纹理1
  diffuseMap = loadTexture("../static/texture/container2.png", width1, height1, channels1);
  // 设置特殊参数（仅对texture1）
  glBindTexture(GL_TEXTURE_2D, diffuseMap);
  float borderColor[] = {0.3f, 0.1f, 0.7f, 1.0f};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
  // 纹理2
  specularMap = loadTexture("../static/texture/container2_specular.png", width2, height2, channels2);

  // 渲染循环
  while (!glfwWindowShouldClose(window))
  {
    // 计算时间差，用于计算帧率以限制相机因时间变化而移动过快
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    // 处理输入（使用最新的deltaTime）
    processInput(window);

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
        // ImGui::ColorEdit3("Material_DiffuseColor", (float*)&materialDiffuseColor);
        // ImGui::ColorEdit3("Material_SpecularColor", (float*)&materialSpecularColor);
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
    ourShader.setInt(locMaterialDiffuseColor, materialDiffuseColor);
    ourShader.setInt(locMaterialSpecularColor, materialSpecularColor); 
    ourShader.setFloat(locMaterialShininess, shininess);

    // 光源属性传入着色器中
    glm::vec3 lightPos = glm::vec3(lightPosition.x * glm::sin(glfwGetTime()), lightPosition.y, lightPosition.z);
    ourShader.setVec3(locLightPos, cameraPos);
    ourShader.setVec3(locLightDirection, cameraFront);
    // 重要：平行光方向（也就是光照方向，只是不需要计算每一个着色点指向光源的向量了，直接使用光照方向，因为平行光的光源方向是固定的，所有着色点都使用同一个光照方向）
    ourShader.setVec3(locParallelLightDirection, lightPos);
    ourShader.setVec3(locLightColor, lightColor);
    // 聚光灯属性传入着色器中
    ourShader.setFloat(locCutOff, cutOff);
    ourShader.setFloat(locOuterCutOff, outerCutOff);
    // 光强度属性传入着色器中
    ourShader.setVec3(locLightAmbientStrength, lightAmbientStrength);  // 环境光强度
    ourShader.setVec3(locLightDiffuseStrength, lightDiffuseStrength);  // 漫反射强度
    ourShader.setVec3(locLightSpecularStrength, lightSpecularStrength); // 镜面反射强度
    // 衰减属性传入着色器中
    ourShader.setFloat(locLightAttenuationConstant, lightAttenuationConstant); // 衰减常数项
    ourShader.setFloat(locLightAttenuationLinear, lightAttenuationLinear); // 衰减一次项
    ourShader.setFloat(locLightAttenuationQuadratic, lightAttenuationQuadratic); // 衰减二次项

    // 激活纹理单元，并绑定纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseMap);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, specularMap);

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

    // 设置立方体位置
    for(unsigned int i = 0; i < sizeof(cubePositions) / sizeof(cubePositions[0]); i++)
    {
      // 创建模型矩阵
      glm::mat4 model = glm::mat4(1.0f);
      // 根据位置数组来平移立方体
      model = glm::translate(model, cubePositions[i]);
      // 使用四元数让立方体自旋转
      glm::qua<float> quat = glm::quat(glm::vec3(factor, factor, factor) * 0.5f);
      model = model * glm::mat4_cast(quat);
      ourShader.setMat4(locModel, model);

      // 绘制立方体
      glDrawElements(GL_TRIANGLES, boxGeometry.indices.size(), GL_UNSIGNED_INT, 0);
    }

    // 绘制灯光物体
    lightObjectShader.use();
    glm::mat4 lightModel = glm::mat4(1.0f);
    lightModel = glm::translate(lightModel, cameraPos); // 移动到设置好的灯光位置
    lightObjectShader.setMat4(locLightModel, lightModel);
    lightObjectShader.setMat4(locLightView, view);
    lightObjectShader.setMat4(locLightProjection, projection);
    glBindVertexArray(sphereGeometry.VAO);
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