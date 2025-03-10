#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <tool/Shader.h>
#include <geometry/PlaneGeometry.h>
#include <geometry/BoxGeometry.h>
#include <geometry/SphereGeometry.h>

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <tool/stb_image.h>

// 着色器代码
const char *vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TexCoords;

out vec3 outPosition;
out vec2 outTexCoord;

uniform float factor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
  gl_Position = projection * view * model * vec4(Position, 1.0f);
  gl_PointSize = 10.0f;
  outTexCoord = TexCoords;
}
)";

const char *fragmentShaderSource =  R"(
#version 330 core
out vec4 FragColor;
in vec2 outTexCoord;

uniform sampler2D texture1;
uniform sampler2D texture2;

void main() {
  FragColor = mix(texture(texture1, outTexCoord), texture(texture2, outTexCoord), 0.2);
  // FragColor = texture(texture1, outTexCoord);
}
)";


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// 检查用户是否按下了返回键(Esc)
void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        std::cout<< "Esc is pressed" << std::endl;
        glfwSetWindowShouldClose(window, true);
    }
        
}

const unsigned int SCREEN_WIDTH = 800;
const unsigned int SCREEN_HEIGHT = 600;

using namespace std;

int main()
{
  glfwInit();
  // 设置主要和次要版本
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

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }
  // 设置视口
  glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  // 渲染相关设置：
  // 设置背景颜色
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  // glClearColor(25.0f / 255.0f, 25.0f / 255.0f, 25.0f / 255.0f, 1.0f);
  // 启用点大小
  glEnable(GL_PROGRAM_POINT_SIZE);
  // // 启用混合
  // glEnable(GL_BLEND);
  // // 设置混合函数
  // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  // 设置绘制模式
  // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  // 开启深度测试
  glEnable(GL_DEPTH_TEST);

  // 注册窗口变化监听
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // 创建着色器（包括顶点着色器、片段着色器、着色器程序、uniform设置）
  Shader ourShader = Shader::FromSource(vertexShaderSource, fragmentShaderSource);

  // 获取uniform变量location
  GLint texture1Location = ourShader.getUniformLocation("texture1");
  GLint texture2Location = ourShader.getUniformLocation("texture2");
  float factor = 0.0;
  GLint locFactor = ourShader.getUniformLocation("factor");
  GLint locModel = ourShader.getUniformLocation("model");
  GLint locView = ourShader.getUniformLocation("view");
  GLint locProjection = ourShader.getUniformLocation("projection");

  // 创建平面几何体（包括几何的顶点、index、VAO、VBO、EBO）
  PlaneGeometry planeGeometry(1.0f, 1.0f, 1.0f, 1.0f);
  // 创建立方体几何体
  BoxGeometry boxGeometry(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
  // 创建球体
  SphereGeometry sphereGeometry(0.5, 20.0, 20.0);

  // 生成纹理
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

    // 渲染指令
    // 清除颜色缓冲和深度缓冲
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ourShader.use();
    ourShader.setInt(texture1Location, 0);
    ourShader.setInt(texture2Location, 1);
    factor = glfwGetTime();
    ourShader.setFloat(locFactor, factor * 0.2f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture1);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture2);

    // 绑定VAO
    // 先绑定立方体
    glBindVertexArray(boxGeometry.VAO);

    // 创建视图矩阵view
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, -5.0f));
    ourShader.setMat4(locView, view);

    // 创建投影矩阵projection
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(45.0f), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
    ourShader.setMat4(locProjection, projection);

    // 设置立方体位置
    for(unsigned int i = 0; i < sizeof(cubePositions) / sizeof(cubePositions[0]); i++)
    {
      // 创建模型矩阵
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, cubePositions[i]);

      float angle = 20.0f * i;
      // 由于第一个i是0，导致angle第一个也是0，所以需要先，有一个基础旋转。
      model = glm::rotate(model, factor * glm::radians(-55.0f), glm::vec3(1.0f, 1.0f, 1.0f));
      // 在添加一个旋转角度，这样可以让每个旋转都不同。
      model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

      // 缩小一点
      model = glm::scale(model, glm::vec3(0.3, 0.3, 0.3));

      ourShader.setMat4(locModel, model);

      // 绘制立方体
      glDrawElements(GL_TRIANGLES, boxGeometry.indices.size(), GL_UNSIGNED_INT, 0);
    }

    // 绑定平面
    glBindVertexArray(planeGeometry.VAO);
    // 更新模型矩阵
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.0, 0.0, 0.0));
    model = glm::rotate(model, factor * glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    ourShader.setMat4(locModel, model);
    glDrawElements(GL_TRIANGLES, planeGeometry.indices.size(), GL_UNSIGNED_INT, 0);

    // 绑定球体
    glBindVertexArray(sphereGeometry.VAO);
    // 更新模型矩阵
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(1.0, 0.0, 0.0));
    model = glm::rotate(model, factor * glm::radians(45.0f), glm::vec3(1.0f, 0.5f, 0.5f));
    ourShader.setMat4(locModel, model);
    glDrawElements(GL_TRIANGLES, sphereGeometry.indices.size(), GL_UNSIGNED_INT, 0);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // 释放资源
  // planeGeometry.dispose();
  boxGeometry.dispose();
  glfwTerminate();
  return 0;
}
