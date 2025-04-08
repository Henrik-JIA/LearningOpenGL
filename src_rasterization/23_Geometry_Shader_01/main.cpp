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

// #include <tool/mesh.h>
#include <tool/model.h>

// 着色器代码
const char *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out VS_OUT {
    vec3 color;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vs_out.color = aColor;
    gl_Position = projection * view * model * vec4(aPos.x, aPos.y, aPos.z, 1.0); 
    gl_PointSize = 15.0; // 设置点的大小
}
)";

const char *geometryShaderSource = R"(
#version 330 core
layout (points) in; // 声明从顶点着色器输入的图元类型，输入的图元是点
layout (triangle_strip, max_vertices = 5) out; // 声明几何着色器输出的图元类型为三角形带，最大顶点数为5

in VS_OUT {
    vec3 color; // 从顶点着色器传入的颜色
} gs_in[]; // 输入的必须是一个数组，实际应为[1]，但[ ]表示自动匹配长度

out vec3 fColor; // 输出颜色

void build_house(vec4 position)
{    
    fColor = gs_in[0].color; // gs_in[0]  因为只有一个输入顶点

    // 生成顶点
    // 新生成的五个顶点均是基于输入点位置的偏移的，并且前四个顶点颜色为输入顶点的颜色，第五个顶点颜色为白色
    // 新生成的五个顶点以三角形带triangle_strip的形式输出。
    gl_Position = position + vec4(-0.2, -0.2, 0.0, 0.0); // 1:bottom-left   
    EmitVertex(); // 生成顶点
    gl_Position = position + vec4( 0.2, -0.2, 0.0, 0.0); // 2:bottom-right
    EmitVertex();
    gl_Position = position + vec4(-0.2,  0.2, 0.0, 0.0); // 3:top-left
    EmitVertex();
    gl_Position = position + vec4( 0.2,  0.2, 0.0, 0.0); // 4:top-right
    EmitVertex();
    
    gl_Position = position + vec4( 0.0,  0.4, 0.0, 0.0); // 5:top
    fColor = vec3(1.0, 1.0, 1.0); // 设置颜色
    EmitVertex();

    // 结束
    EndPrimitive();
}

void main() {    
    vec4 position = gl_in[0].gl_Position; // 输入点的原始位置
    // 对每个输入的顶点都执行一般build_house函数的操作
    build_house(position); // 这里的gl_Position我们并没有定义，是内建(Built-in)变量。
}

)";

const char *fragmentShaderSource =  R"(
#version 330 core
out vec4 FragColor;

in vec3 fColor;

void main()
{
    FragColor = vec4(fColor, 1.0); 
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
Camera camera(SCREEN_WIDTH, SCREEN_HEIGHT, glm::vec3(0.0f, 0.0f, 5.0f)); // 相机类
glm::vec3 cameraPos = camera.Position; // 相机位置
glm::vec3 cameraFront = camera.Front; // 相机前向
glm::vec3 cameraUp = camera.Up; // 相机上向

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
  Shader ourShader = Shader::FromSource(vertexShaderSource, fragmentShaderSource, geometryShaderSource);

  // // 加载模型（nanosuit）
  // Model ourModel("../static/model/nanosuit/nanosuit.obj");

  // 定义顶点
  float points[] = {
      -0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f, // 左上
      0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f, // 右上
      0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, // 右下
      -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f  // 左下
  };

  // 创建VAO、VBO
  unsigned int VBO, VAO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  
  // 绑定VAO、VBO
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);

  // 设置VBO数据，填充数据
  glBufferData(GL_ARRAY_BUFFER, sizeof(points), &points, GL_STATIC_DRAW);

  // 顶点属性
  glEnableVertexAttribArray(0); // 启用顶点属性0位置
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0); // 设置顶点属性0的指针
  glEnableVertexAttribArray(1); // 启用顶点属性1颜色
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); // 设置顶点属性1的指针
  glBindVertexArray(0);


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
      
      ImGui::PopStyleVar();  // 恢复样式变量（可选）
      ImGui::End();  // 必须添加的闭合调用
    }

    // 渲染指令
    // 设置背景颜色
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);

    // 清除颜色缓冲和深度缓冲
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 创建视图矩阵view
    glm::mat4 view = glm::mat4(1.0f);
    view = camera.GetViewMatrix();
    ourShader.setMat4("view", view);

    // 创建投影矩阵projection
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(fov), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
    ourShader.setMat4("projection", projection);

    // 创建模型矩阵
    glm::mat4 model = glm::mat4(1.0f);
    // 根据位置数组来平移立方体
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    ourShader.setMat4("model", model);

    // 使用着色器
    ourShader.use();
    // 绘制模型
    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, 4);

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

// 通过滚动鼠标滚轮控制相机沿观察方向移动
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