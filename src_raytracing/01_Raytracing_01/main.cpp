#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <tool/stb_image.h>

#include <tool/Shader.h>
#include <tool/Camera.h>
#include <tool/TimeRecorder.h>
#include <tool/ScreenFBO.h>
#include <tool/Mesh.h>
#include <tool/Model.h>
#include <geometry/RT_Screen_2D.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

timeRecord timeRecorder;

Camera cam(SCR_WIDTH, SCR_HEIGHT, glm::vec3(0.0f, 0.0f, 3.0f));

ScreenFBO screenBuffer;

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
	// 光线追踪着色器，用于获得当前帧的结果
	Shader RayTracerShader = Shader::FromFile("../src_raytracing/01_Raytracing_01/shader/RayTracerVertexShader.glsl", "../src_raytracing/01_Raytracing_01/shader/RayTracerFragmentShader.glsl");
	// 屏幕着色器，用于将当前帧与历史帧的结果合并，并显示到屏幕上。
	Shader ScreenShader = Shader::FromFile("../src_raytracing/01_Raytracing_01/shader/ScreenVertexShader.glsl", "../src_raytracing/01_Raytracing_01/shader/ScreenFragmentShader.glsl");

	// 绑定屏幕的坐标位置
	RT_Screen screen;
	screen.InitScreenBind();

	// 生成屏幕FrameBuffer
	screenBuffer.configuration(SCR_WIDTH, SCR_HEIGHT);
	
	// 渲染大循环
	while (!glfwWindowShouldClose(window))
	{
		// 计算时间
		timeRecorder.updateTime();

		// 输入
		processInput(window);

		// 渲染
		{
			// 绑定自定义FBO
			screenBuffer.Bind(); // 绑定自定义的帧缓冲

			// 清除颜色缓冲区
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			// 设置光线追踪着色器参数
			// 激活光线追踪着色器，获得当前帧的结果
			RayTracerShader.use(); // 使用光线追踪着色器

			RayTracerShader.setVec3("camera.camPos", cam.Position);
			RayTracerShader.setVec3("camera.front", cam.Front);
			RayTracerShader.setVec3("camera.right", cam.Right);
			RayTracerShader.setVec3("camera.up", cam.Up);

			RayTracerShader.setFloat("camera.halfH", cam.halfH);
			RayTracerShader.setFloat("camera.halfW", cam.halfW);

			RayTracerShader.setVec3("camera.leftbottom", cam.LeftBottomCorner);

			// 绘制到FBO的textureColorbuffer
			screen.RenderToFramebuffer(); // 明确表示这是渲染到FBO，绘制到帧缓冲纹理
			
			// 解绑帧缓冲
			screenBuffer.unBind();
		}

		// 渲染到默认Buffer上
		{
			// 清除颜色缓冲区
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			// 激活屏幕着色器，先激活着色器
			ScreenShader.use();

			// 再绑定纹理，绑定textureColorbuffer到纹理单元0
			screenBuffer.BindAsTexture();

			// 最后设置uniform，screenBuffer绑定的纹理被定义为纹理0，所以这里设置片段着色器中的screenTexture为纹理0
			ScreenShader.setInt("screenTexture", 0);

			// 目前没有实际使用，目的是为了记录渲染的轮数，我们在相机类Camera 中定义了 LoopNum，当相机位置和角度变化时，该值归0。每帧在渲染前，该值都会加1
			ScreenShader.setInt("camera.LoopNum", cam.LoopNum);

			// 再绘制一次屏幕，将当前帧与历史帧的结果合并，并显示到屏幕上。
			screen.DrawTextureQuad(); // 明确表示这是绘制纹理四边形
		}

		// 交换Buffer
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// 条件终止
	glfwTerminate();

	// 释放资源
	screenBuffer.Delete();
	screen.Delete();

	return 0;
}

// 按键处理
void processInput(GLFWwindow *window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cam.ProcessKeyboard(FORWARD, timeRecorder.deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cam.ProcessKeyboard(BACKWARD, timeRecorder.deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cam.ProcessKeyboard(LEFT, timeRecorder.deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cam.ProcessKeyboard(RIGHT, timeRecorder.deltaTime);
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
	cam.ProcessRotationByPosition(xpos, ypos);
}

// 设置fov
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	cam.updateFov(static_cast<float>(yoffset));
}


