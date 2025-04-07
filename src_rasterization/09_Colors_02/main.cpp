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
out vec4 FragColor;
in vec2 outTexCoord;
in vec3 outNormal;
in vec3 outFragPos;

uniform sampler2D texture1;
uniform sampler2D texture2;

// 相机位置
uniform vec3 viewPos;
// 光照位置
uniform vec3 lightPos;
// 光照颜色
uniform vec3 lightColor;
// 环境光强度
uniform float ambientStrength;
// 镜面强度
uniform float specularStrength;
// 镜面光高光系数
uniform int shininess;

void main() {
  // 物体颜色
  vec3 objectColor = vec3(1.0f, 0.5f, 0.31f);

  // 混合纹理
  vec4 textureColor = mix(texture(texture1, outTexCoord), texture(texture2, outTexCoord), 0.2);

  // 环境光项
  vec3 ambient = ambientStrength * lightColor;

  // 漫反射项
  vec3 norm = normalize(outNormal);
  vec3 lightDir = normalize(lightPos - outFragPos);
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = diff * lightColor;

  // 镜面光项
  // 计算视线方向（这里所有射线均是向外的）
  vec3 viewDir = normalize(viewPos - outFragPos);
  // 计算反射方向
  vec3 reflectDir = reflect(-lightDir, norm);
  // 计算镜面光强度（幂次项shininess越大，高光范围约集中）
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
  vec3 specular = specularStrength * spec * lightColor;

  // 仅环境光项
  // vec3 result = ambient * objectColor; 
  // vec3 result = ambient * vec3(textureColor);

  // 仅漫反射项
  // vec3 result = diffuse * objectColor;
  // vec3 result = diffuse * vec3(textureColor);

  // 仅镜面光项
  // vec3 result = specular * objectColor;
  // vec3 result = specular * vec3(textureColor);

  // 环境光+漫反射+镜面光
  // vec3 result = (ambient + diffuse + specular) * objectColor;
  vec3 result = (ambient + diffuse + specular) * vec3(textureColor);
  
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

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void mouse_button_calback(GLFWwindow *window, int button, int action, int mods);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

// 屏幕宽高
int SCREEN_WIDTH = 800;
int SCREEN_HEIGHT = 600;

// 计算时间差
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// 鼠标上一帧的位置
bool isMousePressed = false;
bool isRightMousePressed = false;
double lastX = SCREEN_WIDTH / 2.0f;
double lastY = SCREEN_HEIGHT / 2.0f;

// 视锥体的角度
float fov = 60.0f; 

// 背景颜色
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

// 相机系统
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
glm::vec3 cameraPos = camera.Position;
glm::vec3 cameraFront = camera.Front;
glm::vec3 cameraUp = camera.Up;

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
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetMouseButtonCallback(window, mouse_button_calback);
  glfwSetScrollCallback(window, scroll_callback);

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
  Shader ourShader = Shader::FromSource(vertexShaderSource, fragmentShaderSource);
  Shader lightObjectShader = Shader::FromSource(light_object_vert, light_object_frag);;

  // 获取uniform变量location
  float factor = 0.0;
  GLint locFactor = ourShader.getUniformLocation("factor");
  GLint locLightPos = ourShader.getUniformLocation("lightPos");
  GLint texture1Location = ourShader.getUniformLocation("texture1");
  GLint texture2Location = ourShader.getUniformLocation("texture2");
  GLint locModel = ourShader.getUniformLocation("model");
  GLint locView = ourShader.getUniformLocation("view");
  GLint locProjection = ourShader.getUniformLocation("projection");

  glm::vec3 lightPosition = glm::vec3(1.2f, 1.0f, 2.0f); // 光照位置
  glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f); // 光照颜色
  GLint locLightColor = ourShader.getUniformLocation("lightColor");
  GLint locViewPos = ourShader.getUniformLocation("viewPos");
  float ambientStrength = 0.5f;
  GLint locAmbientStrength = ourShader.getUniformLocation("ambientStrength");
  float specularStrength = 0.5f;
  GLint locSpecularStrength = ourShader.getUniformLocation("specularStrength");
  int shininess = 32;
  GLint locShininess = ourShader.getUniformLocation("shininess");

  GLint locLightModel = lightObjectShader.getUniformLocation("model");
  GLint locLightView = lightObjectShader.getUniformLocation("view");
  GLint locLightProjection = lightObjectShader.getUniformLocation("projection");

  PlaneGeometry planeGeometry(1.0f, 1.0f, 1.0f, 1.0f); // 创建平面几何体（包括几何的顶点、index、VAO、VBO、EBO）
  BoxGeometry boxGeometry(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f); // 创建立方体几何体
  SphereGeometry sphereGeometry(0.1, 10.0, 10.0); // 创建球体 

  // 纹理加载
  unsigned int texture1, texture2;
  glGenTextures(1, &texture1);
  glBindTexture(GL_TEXTURE_2D, texture1);

  // 设置环绕和过滤方式
  float borderColor[] = {0.3f, 0.1f, 0.7f, 1.0f};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // 图像y轴翻转，因为OpenGL要求y轴0.0坐标是在图片的底部的，但是图片的y轴0.0坐标通常在顶部。
  stbi_set_flip_vertically_on_load(true);

  // 加载图片
  int width, height, nrChannels;
  unsigned char *data = stbi_load("../static/texture/container.jpg", &width, &height, &nrChannels, 0);

  if (data)
  {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
  }
  stbi_image_free(data);

  glGenTextures(1, &texture2);
  glBindTexture(GL_TEXTURE_2D, texture2);

  // 设置环绕和过滤方式
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // 加载图片
  data = stbi_load("../static/texture/awesomeface.png", &width, &height, &nrChannels, 0);

  if (data)
  {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
  }
  stbi_image_free(data);

  // 创建多个立方体
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

  while (!glfwWindowShouldClose(window))
  {
    processInput(window);

    // 计算时间差
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // 开始ImGui框架
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
      ImGui::Begin("Hello, world!");
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
      ImGui::ColorEdit3("clear color", (float*)&clear_color);
      ImGui::SliderFloat("fov", &fov, 15.0f, 90.0f);
      ImGui::SliderInt("SCREEN_WIDTH", &SCREEN_WIDTH, 100, 1920);
      ImGui::SliderInt("SCREEN_HEIGHT", &SCREEN_HEIGHT, 100, 1080);
      ImGui::ColorEdit3("Light Color", (float*)&lightColor);
      ImGui::SliderFloat("AmbientStrength", &ambientStrength, 0.1f, 2.0f);
      ImGui::SliderFloat("SpecularStrength", &specularStrength, 0.1f, 2.0f);
      ImGui::SliderInt("Shininess", &shininess, 1, 256);
      ImGui::End();
    }

    // 渲染指令
    // 设置背景颜色
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);

    // 清除颜色缓冲和深度缓冲
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ourShader.use();    
    factor = glfwGetTime();
    ourShader.setFloat(locFactor, factor * 0.2f);

    glm::vec3 lightPos = glm::vec3(lightPosition.x * glm::sin(glfwGetTime()), lightPosition.y, lightPosition.z);
    ourShader.setVec3(locLightPos, lightPos);

    ourShader.setVec3(locViewPos, camera.Position);

    ourShader.setInt(texture1Location, 0);
    ourShader.setInt(texture2Location, 1);
    ourShader.setVec3(locLightColor, lightColor);
    ourShader.setFloat(locAmbientStrength, ambientStrength);
    ourShader.setFloat(locSpecularStrength, specularStrength);
    ourShader.setInt(locShininess, shininess);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture1);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture2);

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
      glm::qua<float> quat = glm::quat(glm::vec3(factor, factor, factor));
      model = model * glm::mat4_cast(quat);
      ourShader.setMat4(locModel, model);

      // 绘制立方体
      glDrawElements(GL_TRIANGLES, boxGeometry.indices.size(), GL_UNSIGNED_INT, 0);
    }

    // 绘制灯光物体
    lightObjectShader.use();
    glm::mat4 lightModel = glm::mat4(1.0f);
    lightModel = glm::translate(lightModel, lightPos);
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
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // 反转Y轴

        lastX = xpos;
        lastY = ypos;

        camera.ProcessMouseMovement(xoffset, yoffset);
        cameraPos = camera.Position;
        cameraFront = camera.Front;
    }
    // 右键：视角平移
    else if (isRightMousePressed) {
        float sensitivity = 0.03f;
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