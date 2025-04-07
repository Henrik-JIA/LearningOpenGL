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
#include "../Source/Tool.h"
#include "../Source/BVHTree.h"
#include "../Source/ObjectTexture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../Tool/stb_image.h"

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

unsigned int SCR_WIDTH = 1200;
unsigned int SCR_HEIGHT = 800;

timeRecord tRecord;

Camera cam(SCR_WIDTH, SCR_HEIGHT);

RenderBuffer screenBuffer;

BVHTree bvhTree;

ObjectTexture ObjTex;

// RayTracerShader 纹理序号：
// 纹理0：Framebuffer
// 纹理1：MeshVertex
// 纹理2：MeshFaceIndex

// ScreenShader 纹理序号：
// 纹理0：Framebuffer

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
	Shader RayTracerShader("./Source/RayTracerVertexShader.glsl", "./Source/RayTracerFragmentShader.glsl");
	Shader ScreenShader("./Source/ScreenVertexShader.glsl", "./Source/ScreenFragmentShader.glsl");

	// 绑定屏幕的坐标位置
	RT_Screen screen;
	screen.InitScreenBind();

	// 生成屏幕FrameBuffer
	screenBuffer.Init(SCR_WIDTH, SCR_HEIGHT);
	
	// 加载数据纹理
	//Model bunny("./Data/bunny.obj");
	Model dragon("./Data/dragon.obj");
	getTexture(dragon.meshes, RayTracerShader, ObjTex, bvhTree, 0.04, glm::vec3(0.0,-0.2,0.0));

	//Model box("./Data/box.obj");
	//getTexture(box.meshes, RayTracerShader, ObjTex, bvhTree, 0.2, glm::vec3(0.0, 0.0, 0.0));

	//测试BVH树
	BVHTest(bvhTree, cam);
	bvhTree.releaseAll();

	// 渲染大循环
	while (!glfwWindowShouldClose(window))
	{
		// 计算时间
		tRecord.updateTime();

		// 输入
		processInput(window);

		// 渲染循环加1
		cam.LoopIncrease();

		// 光线追踪渲染当前帧
		{
			// 绑定到当前帧缓冲区
			screenBuffer.setCurrentBuffer(cam.LoopNum);

			// screenBuffer绑定的纹理被定义为纹理0，所以这里设置片段着色器中的historyTexture为纹理0
			RayTracerShader.setInt("historyTexture", 0);

			// MeshTex赋值
			ObjTex.setTex(RayTracerShader);

			// 激活着色器
			RayTracerShader.use();


			// 相机参数赋值
			RayTracerShader.setVec3("camera.camPos", cam.cameraPos);
			RayTracerShader.setVec3("camera.front", cam.cameraFront);
			RayTracerShader.setVec3("camera.right", cam.cameraRight);
			RayTracerShader.setVec3("camera.up", cam.cameraUp);
			RayTracerShader.setFloat("camera.halfH", cam.halfH);
			RayTracerShader.setFloat("camera.halfW", cam.halfW);
			RayTracerShader.setVec3("camera.leftbottom", cam.LeftBottomCorner);
			RayTracerShader.setInt("camera.LoopNum", cam.LoopNum);

			// 随机数初值赋值
			RayTracerShader.setFloat("randOrigin", 874264.0f * (GetCPURandom() + 1.0f));

			// 球物体赋值
			RayTracerShader.setFloat("sphere[0].radius", 0.5);
			RayTracerShader.setVec3("sphere[0].center", glm::vec3(0.0, 0.0, -1.0));
			RayTracerShader.setInt("sphere[0].materialIndex", 0);
			RayTracerShader.setVec3("sphere[0].albedo", glm::vec3(0.8, 0.7, 0.2));

			RayTracerShader.setFloat("sphere[1].radius", 0.5);
			RayTracerShader.setVec3("sphere[1].center", glm::vec3(1.0, 0.0, -1.0));
			RayTracerShader.setInt("sphere[1].materialIndex", 1);
			RayTracerShader.setVec3("sphere[1].albedo", glm::vec3(0.2, 0.7, 0.6));

			RayTracerShader.setFloat("sphere[2].radius", 0.5);
			RayTracerShader.setVec3("sphere[2].center", glm::vec3(-1.0, 0.0, -1.0));
			RayTracerShader.setInt("sphere[2].materialIndex", 1);
			RayTracerShader.setVec3("sphere[2].albedo", glm::vec3(0.1, 0.3, 0.7));

			RayTracerShader.setFloat("sphere[3].radius", 0.5);
			RayTracerShader.setVec3("sphere[3].center", glm::vec3(0.0, 0.0, 0.0));
			RayTracerShader.setInt("sphere[3].materialIndex", 0);
			RayTracerShader.setVec3("sphere[3].albedo", glm::vec3(0.9, 0.9, 0.9));
			// 三角形赋值
			RayTracerShader.setVec3("tri[0].v0", glm::vec3(2.0, -0.5, 2.0));
			RayTracerShader.setVec3("tri[0].v1", glm::vec3(-2.0, -0.5, -2.0));
			RayTracerShader.setVec3("tri[0].v2", glm::vec3(-2.0, -0.5, 2.0));
			
			RayTracerShader.setVec3("tri[1].v0", glm::vec3(2.0, -0.5, 2.0));
			RayTracerShader.setVec3("tri[1].v1", glm::vec3(-2.0, -0.5, -2.0));
			RayTracerShader.setVec3("tri[1].v2", glm::vec3(2.0, -0.5, -2.0));

			

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
	cam.updateCameraFront(xpos, ypos);
}

// 设置fov
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	cam.updateFov(static_cast<float>(yoffset));
}







