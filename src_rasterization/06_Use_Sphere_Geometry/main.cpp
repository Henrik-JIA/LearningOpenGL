#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <tool/Shader.h>
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

/**
  * |  1   0      0     0|    |x|    |         x         |
  * |  0   cos0  -sin0  0|  * |y| =  |cos0 * y - sin0 * z|
  * |  0   sin0  cos0   0|    |z|    |sin0 * y + cos0 * z|
  * |  0   0      0     1|    |w|    |         1         |
  */
mat4 rotateX(float _angle) {
  return mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, cos(_angle), -sin(_angle), 0.0f, 0.0f, sin(_angle), cos(_angle), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

/**
  * |  cos0  0  sin0   0|    |x|    |x * cos0 + z * sin0|
  * |  0     0     1   0|  * |y| =  |         y         |
  * | -sin0  0  cos0   0|    |z|    |-sinθ *x + cosθ * z|
  * |  0     0     0   1|    |w|    |         1         |
  */
mat4 rotateY(float _angle) {
  return mat4(cos(_angle), 0.0f, sin(_angle), 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -sin(_angle), 0.0f, cos(_angle), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

/**
  * |  cos0  -sin0   0   0|    |x|    |x * cos0 - y * sin0|
  * |  sin0   cos0   0   0|  * |y| =  |x * sin0 + y * cos0|
  * |  0       0     1   0|    |z|    |         z         |
  * |  0       0     0   1|    |w|    |         1         |
  */
mat4 rotateZ(float _angle) {
  return mat4(cos(_angle), -sin(_angle), 0.0f, 0.0f, sin(_angle), cos(_angle), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

mat4 rotateXYZ(float angle) {
  return rotateZ(angle) * rotateY(angle) * rotateX(angle);
}

void main() {

  // mat4 rotate = rotateZ(factor) * rotateY(factor) * rotateX(factor);

  gl_Position = rotateXYZ(factor) * vec4(Position, 1.0f);

  // gl_Position = vec4(Position, 1.0f);
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
  FragColor = mix(texture(texture1, outTexCoord), texture(texture2, outTexCoord), 0.1);
  
  // vec2 coord = gl_PointCoord - vec2(0.5);
  // float r = float(length(coord) > 0.5);
  //// FragColor = vec4(1.0, 0.6, 0.1, 0.3);
  // FragColor = vec4(0.0, 0.91, 0.9, (1 - r) * 0.1);
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
  // 设置背景颜色
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  // glClearColor(25.0f / 255.0f, 25.0f / 255.0f, 25.0f / 255.0f, 1.0f);
  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // 启用深度测试
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  // 注册窗口变化监听
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  Shader ourShader = Shader::FromSource(vertexShaderSource, fragmentShaderSource);

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

  // 图像y轴翻转
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
  data = stbi_load("../static/texture/dot.png", &width, &height, &nrChannels, 0);

  if (data)
  {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
  }
  stbi_image_free(data);
  ourShader.use();
  ourShader.setInt("texture1", 0);
  ourShader.setInt("texture2", 1);

  float factor = 0.0;

  while (!glfwWindowShouldClose(window))
  {
    processInput(window);

    // 渲染指令
    // 清除颜色缓冲
    // glClear(GL_COLOR_BUFFER_BIT);
    // 同时清除颜色和深度缓冲
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ourShader.use();

    factor = glfwGetTime();
    ourShader.setFloat("factor", -factor * 0.3);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture1);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture2);

    glBindVertexArray(sphereGeometry.VAO);

    glDrawElements(GL_TRIANGLES, sphereGeometry.indices.size(), GL_UNSIGNED_INT, 0);
    glDrawElements(GL_POINTS, sphereGeometry.indices.size(), GL_UNSIGNED_INT, 0);
    // glDrawElements(GL_LINE_LOOP, sphereGeometry.indices.size(), GL_UNSIGNED_INT, 0);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  sphereGeometry.dispose();
  glfwTerminate();
  return 0;
}