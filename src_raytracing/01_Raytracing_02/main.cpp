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
#include <tool/RandomUtils.h>
#include <tool/TimeRecorder.h>
#include <tool/RenderBuffer.h>
#include <geometry/RT_Screen_2D.h>


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
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// 加载所有的OpenGL函数指针
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// CPU随机数初始化
	CPURandomInit();

	// 加载着色器
	Shader RayTracerShader = Shader::FromFile("../src_raytracing/01_Raytracing_02/shader/RayTracerVertexShader.glsl", "../src_raytracing/01_Raytracing_02/shader/RayTracerFragmentShader.glsl");
	Shader ScreenShader = Shader::FromFile("../src_raytracing/01_Raytracing_02/shader/ScreenVertexShader.glsl", "../src_raytracing/01_Raytracing_02/shader/ScreenFragmentShader.glsl");

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

			RayTracerShader.setVec3("camera.camPos", cam.Position);
			RayTracerShader.setVec3("camera.front", cam.Front);
			RayTracerShader.setVec3("camera.right", cam.Right);
			RayTracerShader.setVec3("camera.up", cam.Up);
			
			RayTracerShader.setFloat("camera.halfH", cam.halfH);
			RayTracerShader.setFloat("camera.halfW", cam.halfW);
			
			RayTracerShader.setVec3("camera.leftbottom", cam.LeftBottomCorner);
			
			RayTracerShader.setInt("camera.LoopNum", cam.LoopNum);
			
			RayTracerShader.setFloat("randOrigin", 674764.0f * (GetCPURandom() + 1.0f));
			
			// 绘制到FBO的textureColorbuffer
			screen.RenderToFramebuffer(); // 明确表示这是渲染到FBO，绘制到帧缓冲纹理

			// 解绑帧缓冲，解绑到默认帧缓冲
			screenBuffer.unBind();
		}

		// 渲染到默认Buffer上
		{			
			// 清屏
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			// 先激活着色器
			ScreenShader.use();

			// 将之前FBO渲染的纹理传入
			screenBuffer.setCurrentAsTexture(cam.LoopNum);
			// 最后设置uniform，screenBuffer绑定的纹理被定义为纹理0，所以这里设置片段着色器中的screenTexture为纹理0
			ScreenShader.setInt("screenTexture", 0);

			// 设置模型、视图、投影矩阵
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); 
			model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // y轴旋转45度
			glm::mat4 view = cam.GetViewMatrix();
			glm::mat4 projection = glm::perspective(glm::radians(cam.fov), (float)SCR_WIDTH/(float)SCR_HEIGHT, 0.1f, 100.0f);
			ScreenShader.setMat4("model", model);
			ScreenShader.setMat4("view", view);
			ScreenShader.setMat4("projection", projection);

			// 绘制屏幕
			screen.DrawTextureQuad();
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
