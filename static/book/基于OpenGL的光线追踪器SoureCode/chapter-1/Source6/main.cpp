#include "../glad/glad.h"
#include "../GLFW/glfw3.h"

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "../Source/Shader.h"
#include "../Source/Mesh.h"
#include "../Source/Model.h"
#include "../Source/Camera.h"
#include "../Source/RT_Screen.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../Tool/stb_image.h"

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

timeRecord time;

Camera cam(SCR_WIDTH, SCR_HEIGHT);

int main()
{
	// GLFW初始化
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// 创建GLFW窗口
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	// 交互事件
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// 窗口捕获鼠标，不显示鼠标
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// 加载所有的OpenGL函数指针
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// 加载着色器
	Shader ourShader("./Source/vertexShader.glsl", "./Source/fragmentShader.glsl");

	// 绑定屏幕的坐标位置
	RT_Screen screen;
	screen.InitScreenBind();

	// 渲染大循环
	while (!glfwWindowShouldClose(window))
	{
		// 计算时间
		time.updateTime();

		// 输入
		processInput(window);

		// 渲染
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// 激活着色器
		ourShader.use();
		ourShader.setVec3("camera.camPos", cam.cameraPos);
		ourShader.setVec3("camera.front", cam.cameraFront);
		ourShader.setVec3("camera.right", cam.cameraRight);
		ourShader.setVec3("camera.up", cam.cameraUp);

		ourShader.setFloat("camera.halfH", cam.halfH);
		ourShader.setFloat("camera.halfW", cam.halfW);

		ourShader.setVec3("camera.leftbottom", cam.LeftBottomCorner);

		// 渲染屏幕buffer
		screen.DrawScreen();

		// 交换Buffer
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// 条件终止
	glfwTerminate();
	return 0;
}

// 按键处理
void processInput(GLFWwindow *window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cam.ProcessKeyboard(FORWARD, time.deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cam.ProcessKeyboard(BACKWARD, time.deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cam.ProcessKeyboard(LEFT, time.deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cam.ProcessKeyboard(RIGHT, time.deltaTime);
}

// 处理窗口尺寸变化
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	SCR_WIDTH = width;
	SCR_HEIGHT = height;
	cam.updateScreenRatio(SCR_WIDTH, SCR_HEIGHT);
	glViewport(0, 0, width, height);
}

// 鼠标事件响应
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);
	cam.updateCameraFront(xpos, ypos);
}

// 设置fov
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	cam.updateFov(static_cast<float>(yoffset));
}


