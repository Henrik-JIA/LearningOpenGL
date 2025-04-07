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

#define STB_IMAGE_IMPLEMENTATION
#include "../Tool/stb_image.h"

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
	// GLFW��ʼ��
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// ����GLFW����
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	// �����¼�
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// ���ڲ�����꣬����ʾ���
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// �������е�OpenGL����ָ��
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// CPU�������ʼ��
	CPURandomInit();

	// ������ɫ��
	Shader RayTracerShader("./Source/RayTracerVertexShader.glsl", "./Source/RayTracerFragmentShader.glsl");
	Shader ScreenShader("./Source/ScreenVertexShader.glsl", "./Source/ScreenFragmentShader.glsl");

	// ����Ļ������λ��
	RT_Screen screen;
	screen.InitScreenBind();

	// ������ĻFrameBuffer
	screenBuffer.Init(SCR_WIDTH, SCR_HEIGHT);
	
	// ��Ⱦ��ѭ��
	while (!glfwWindowShouldClose(window))
	{
		// ����ʱ��
		tRecord.updateTime();

		// ����
		processInput(window);

		// ��Ⱦѭ����1
		cam.LoopIncrease();

		// ����׷����Ⱦ��ǰ֡
		{
			// �󶨵���ǰ֡������
			screenBuffer.setCurrentBuffer(cam.LoopNum);

			// ������ɫ��
			RayTracerShader.use();
			// screenBuffer�󶨵���������Ϊ����0��������������Ƭ����ɫ���е�historyTextureΪ����0
			RayTracerShader.setInt("historyTexture", 0);

			RayTracerShader.setVec3("camera.camPos", cam.cameraPos);
			RayTracerShader.setVec3("camera.front", cam.cameraFront);
			RayTracerShader.setVec3("camera.right", cam.cameraRight);
			RayTracerShader.setVec3("camera.up", cam.cameraUp);
			RayTracerShader.setFloat("camera.halfH", cam.halfH);
			RayTracerShader.setFloat("camera.halfW", cam.halfW);
			RayTracerShader.setVec3("camera.leftbottom", cam.LeftBottomCorner);
			RayTracerShader.setInt("camera.LoopNum", cam.LoopNum);
			RayTracerShader.setFloat("randOrigin", 674764.0f * (GetCPURandom() + 1.0f));
			
			// ��ȾFrameBuffer
			screen.DrawScreen();
		}

		// ��Ⱦ��Ĭ��Buffer��
		{
			// �󶨵�Ĭ�ϻ�����
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			// ����
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			ScreenShader.use();
			screenBuffer.setCurrentAsTexture(cam.LoopNum);
			// screenBuffer�󶨵���������Ϊ����0��������������Ƭ����ɫ���е�screenTextureΪ����0
			ScreenShader.setInt("screenTexture", 0);

			// ������Ļ
			screen.DrawScreen();
		}

		// ����Buffer
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// ������ֹ
	glfwTerminate();

	// �ͷ���Դ
	screenBuffer.Delete();
	screen.Delete();

	return 0;
}

// ��������
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

// �����ڳߴ�仯
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	SCR_WIDTH = width;
	SCR_HEIGHT = height;
	cam.updateScreenRatio(SCR_WIDTH, SCR_HEIGHT);
	glViewport(0, 0, width, height);
}

// ����¼���Ӧ
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);
	cam.updateCameraFront(xpos, ypos);
}

// ����fov
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	cam.updateFov(static_cast<float>(yoffset));
}


