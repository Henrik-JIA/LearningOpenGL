#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <tool/stb_image.h>

#include <tool/Shader.h>
#include <tool/Mesh.h>
#include <tool/Model.h>
#include <tool/Camera.h>
#include <tool/TimeRecorder.h>
#include <tool/RandomUtils.h> // 这个就对应Tool.h文件
#include <tool/RenderBuffer.h> // 这个就对应RT_Screen.h文件
#include <geometry/RT_Screen_2D.h> // 这个就对应RT_Screen.h文件

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

timeRecord tRecord;

Camera cam(SCR_WIDTH, SCR_HEIGHT);

RenderBuffer screenBuffer;

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
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// 加载所有的OpenGL函数指针
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// CPU随机数初始化
	CPURandomInit();

	// 加载着色器
	Shader RayTracerShader = Shader::FromFile("../src_raytracing/01_Raytracing_03/shader/RayTracerVertexShader.glsl", "../src_raytracing/01_Raytracing_03/shader/RayTracerFragmentShader.glsl");
	Shader ScreenShader = Shader::FromFile("../src_raytracing/01_Raytracing_03/shader/ScreenVertexShader.glsl", "../src_raytracing/01_Raytracing_03/shader/ScreenFragmentShader.glsl");

	// 绑定屏幕的坐标位置
	RT_Screen screen;
	screen.InitScreenBind();

	// 生成屏幕FrameBuffer
	screenBuffer.Init(SCR_WIDTH, SCR_HEIGHT);
	
	// 渲染大循环
	while (!glfwWindowShouldClose(window))
	{
		// 计算时间
		tRecord.updateTime();

		// 输入
		processInput(window);

		// 渲染循环加1，LoopNum初始为0，循环开始就加1。
		cam.LoopIncrease();

		// 光线追踪渲染当前帧
		{
			// 绑定到当前帧缓冲区
			screenBuffer.setCurrentBuffer(cam.LoopNum);

			// 激活着色器
			RayTracerShader.use();
			// screenBuffer绑定的纹理被定义为纹理0，所以这里设置片段着色器中的historyTexture为纹理0
			RayTracerShader.setInt("historyTexture", 0);
			// 相机参数赋值
			RayTracerShader.setVec3("camera.camPos", cam.Position);
			RayTracerShader.setVec3("camera.front", cam.Front);
			RayTracerShader.setVec3("camera.right", cam.Right);
			RayTracerShader.setVec3("camera.up", cam.Up);

			RayTracerShader.setFloat("camera.halfH", cam.halfH);
			RayTracerShader.setFloat("camera.halfW", cam.halfW);

			RayTracerShader.setVec3("camera.leftbottom", cam.LeftBottomCorner);
			
			RayTracerShader.setInt("camera.LoopNum", cam.LoopNum);
			
			// 随机数初值赋值
			RayTracerShader.setFloat("randOrigin", 674764.0f * (GetCPURandom() + 1.0f));
			
			// 球体数量赋值，绘制10个球体，前9个球体使用循环生成，最后一个球体是地板
			// -----------------------------------------------------
			RayTracerShader.setInt("sphereNum", 10);
			// 使用循环生成前9个球体
			int x_values[] = {0, 1, -1};
			for (int i = 0; i < 9; ++i) {
				int y = i / 3;
				float x = x_values[i % 3];
				RayTracerShader.setFloat(("sphere[" + std::to_string(i) + "].radius").c_str(), 0.5f);
				RayTracerShader.setVec3(("sphere[" + std::to_string(i) + "].center").c_str(), 
					glm::vec3(x, y, -1.0f));
			}
			// 地板物体赋值
			RayTracerShader.setFloat("sphere[9].radius", 100.0);
			RayTracerShader.setVec3("sphere[9].center", glm::vec3(0.0, -100.5, -1.0));
			// -----------------------------------------------------

			// 渲染FrameBuffer
			screen.DrawScreen();
		}

		// 渲染到默认Buffer上
		{
			// 绑定到默认缓冲区
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			// 清屏
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			ScreenShader.use();
			screenBuffer.setCurrentAsTexture(cam.LoopNum);
			// screenBuffer绑定的纹理被定义为纹理0，所以这里设置片段着色器中的screenTexture为纹理0
			ScreenShader.setInt("screenTexture", 0);

			// 绘制屏幕
			screen.DrawScreen();
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
		cam.ProcessKeyboard(FORWARD, tRecord.deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cam.ProcessKeyboard(BACKWARD, tRecord.deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cam.ProcessKeyboard(LEFT, tRecord.deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cam.ProcessKeyboard(RIGHT, tRecord.deltaTime);
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


