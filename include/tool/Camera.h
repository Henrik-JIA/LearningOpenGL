#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement
{
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float KEYBOARD_MOVEMENT_SPEED = 2.5f;
const float ROTATION_SENSITIVITY = 0.2f;
const float PAN_SENSITIVITY = 0.02f;
const float SCROLL_SENSITIVITY = 1.5f;
const float FOV = 22.5f; // ZOOM

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
	// 屏幕宽高
	int ScreenWidth;
	int ScreenHeight;
	// 相机属性
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;
	// euler Angles
	float Yaw;
	float Pitch;
	// camera options
	float KeyBoardMovementSpeed;
	float RotationSensitivity;
	float PanSensitivity;
	float ScrollSensitivity;
	float fov;

	// 鼠标上一帧的位置
	bool isRotating;
	bool isPanning;
	double lastX;
	double lastY;

	// 屏幕宽高比
	float ScreenRatio;
	// 屏幕宽高的一半
	float halfH;
	float halfW;
	// 屏幕左下角坐标
	glm::vec3 LeftBottomCorner;

	// 光线追踪的循环次数
	int LoopNum;

	// 构造函数1：使用向量参数
	// 位置
	// 上方向：Y轴
	// 默认值YAW = -90.0f，初始朝Z负方向
	// 默认值PITCH = 0.0f，平视
	Camera( int ScreenWidth, 
			int ScreenHeight,
			glm::vec3 position = glm::vec3(0.0f, 0.0f, 2.0f), 
			glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), 
			float yaw = YAW, 
			float pitch = PITCH
		) : ScreenWidth(ScreenWidth),
			ScreenHeight(ScreenHeight),
			ScreenRatio((float)ScreenWidth / ScreenHeight),
			halfH(glm::tan(glm::radians(fov))),
			halfW(halfH * ScreenRatio),
			Front(glm::vec3(0.0f, 0.0f, -1.0f)), 
			KeyBoardMovementSpeed(KEYBOARD_MOVEMENT_SPEED), 
			RotationSensitivity(ROTATION_SENSITIVITY),
			PanSensitivity(PAN_SENSITIVITY),
			ScrollSensitivity(SCROLL_SENSITIVITY),
			fov(FOV),
			LoopNum(0),
			isRotating(false),
			isPanning(false),
			lastX(ScreenWidth / 2.0f),
			lastY(ScreenHeight / 2.0f)
	{
		// 直接使用传入的向量
		Position = position;
		WorldUp = up;
		Yaw = yaw;
		Pitch = pitch;
		updateCameraVectors();
	}

	// 构造函数2：使用标量参数
	Camera( int ScreenWidth, 
			int ScreenHeight,
		    float posX, float posY, float posZ, // 相机位置坐标分量（X,Y,Z）
			float upX, float upY, float upZ, // 上方向向量分量（X,Y,Z）
			float yaw, float pitch // 欧拉角（无默认值）
		) : ScreenWidth(ScreenWidth),
			ScreenHeight(ScreenHeight),
			ScreenRatio((float)ScreenWidth / ScreenHeight),
			halfH(glm::tan(glm::radians(fov))),
			halfW(halfH * ScreenRatio),
			Front(glm::vec3(0.0f, 0.0f, -1.0f)), 
			KeyBoardMovementSpeed(KEYBOARD_MOVEMENT_SPEED), 
			RotationSensitivity(ROTATION_SENSITIVITY),
			PanSensitivity(PAN_SENSITIVITY),
			ScrollSensitivity(SCROLL_SENSITIVITY),
			fov(FOV),
			LoopNum(0),
			isRotating(false),
			isPanning(false),
			lastX(ScreenWidth / 2.0f),
			lastY(ScreenWidth / 2.0f)
	{
		// 用标量参数构造向量
		Position = glm::vec3(posX, posY, posZ);
		WorldUp = glm::vec3(upX, upY, upZ);
		Yaw = yaw;
		Pitch = pitch;
		updateCameraVectors();
	}

	// returns the view matrix calculated using Euler Angles and the LookAt Matrix
	// 返回LookAt矩阵，用于设置view矩阵
	glm::mat4 GetViewMatrix()
	{
		return glm::lookAt(Position, Position + Front, Up);
	}

	// processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
	void ProcessKeyboard(Camera_Movement direction, float deltaTime)
	{
		float velocity = KeyBoardMovementSpeed * deltaTime;
		if (direction == FORWARD)
			Position += Front * velocity;
		if (direction == BACKWARD)
			Position -= Front * velocity;
		if (direction == LEFT)
			Position -= Right * velocity;
		if (direction == RIGHT)
			Position += Right * velocity;

		LeftBottomCorner = Front - halfW * Right - halfH * Up;
		LoopNum = 0;
	}

	// 直接处理绝对坐标的版本，用于鼠标旋转
    void ProcessRotationByPosition(float xpos, float ypos) {
        if(!isRotating) return; // 状态检查
        
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // 注意Y轴方向反转
        
        updateMousePosition(xpos, ypos); 
        ProcessRotationByOffset(xoffset, yoffset);
    }

	// 鼠标移动，使用偏移量
	// processes input received from a mouse input system. Expects the offset value in both the x and y direction.
	void ProcessRotationByOffset(float xoffset, float yoffset, GLboolean constrainPitch = true)
	{
		// 直接使用类成员变量控制灵敏度
		xoffset *= RotationSensitivity;
		yoffset *= RotationSensitivity;

		Yaw += xoffset;
		Pitch += yoffset;

		// make sure that when pitch is out of bounds, screen doesn't get flipped
		if (constrainPitch)
		{
			if (Pitch > 89.0f)
				Pitch = 89.0f;
			if (Pitch < -89.0f)
				Pitch = -89.0f;
		}

		// update Front, Right and Up Vectors using the updated Euler angles
		updateCameraVectors();
	}

	// 直接处理绝对坐标的版本，用于鼠标平移
	void ProcessPanByPosition(float xpos, float ypos) 
	{
        if(!isPanning) return; // 状态检查
        
        float xoffset = xpos - lastX;
        float yoffset = ypos - lastY; // 注意这里Y轴方向与旋转不同
        
        updateMousePosition(xpos, ypos); 
        ProcessPanByOffset(xoffset, yoffset);
    }

	// 鼠标平移，使用偏移量
	void ProcessPanByOffset(float xoffset, float yoffset) {
		xoffset *= PanSensitivity;
        yoffset *= PanSensitivity;

        glm::vec3 right = glm::normalize(glm::cross(Front, WorldUp));
        glm::vec3 up = glm::normalize(glm::cross(right, Front));
        
        Position -= right * xoffset;
        Position += up * yoffset;

		// 需要补充的更新
		// LeftBottomCorner = Front - halfW * Right - halfH * Up;
		// 或者调用完整更新
		updateCameraVectors(); 
	}

	// processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
	void ProcessScrollFromFOV(float yoffset)
	{
		fov -= (float)yoffset;
		if (fov < 1.0f)
			fov = 1.0f;
		if (fov > 45.0f)
			fov = 45.0f;

		halfH = glm::tan(glm::radians(fov));
		halfW = halfH * ScreenRatio;

		LeftBottomCorner = Front - halfW * Right - halfH * Up;

		LoopNum = 0;
	}

	// 新增滚动处理函数
    void ProcessScrollFromMovement(bool forward, float deltaTime) {
        float velocity = ScrollSensitivity * deltaTime;
        if(forward) {
            Position += Front * velocity;
        } else {
            Position -= Front * velocity;
        }
        updateCameraVectors(); // 确保更新视口参数
    }

	// 更新屏幕宽高比
	void updateScreenRatio(int ScreenWidth, int ScreenHeight) {
		ScreenRatio = (float)ScreenWidth / (float)ScreenHeight;
		halfW = halfH * ScreenRatio;

		LeftBottomCorner = Front - halfW * Right - halfH * Up;

		LoopNum = 0;
	}

	// 更新视野
	void updateFov(float offset) {
		fov -= (float)offset;
		if (fov < 1.0f)
			fov = 1.0f;
		if (fov > 45.0f)
			fov = 45.0f;

		halfH = glm::tan(glm::radians(fov));
		halfW = halfH * ScreenRatio;

		LeftBottomCorner = Front - halfW * Right - halfH * Up;
		LoopNum = 0;
	}

	// 更新鼠标位置
	void updateMousePosition(float x, float y) {
		lastX = x;
		lastY = y;
	}

	// 渲染循环加1
	void LoopIncrease() {
		LoopNum++;
	}

private:
	// calculates the front vector from the Camera's (updated) Euler Angles
	void updateCameraVectors()
	{
		// calculate the new Front vector
		glm::vec3 front = glm::vec3(1.0f);
		front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		front.y = sin(glm::radians(Pitch));
		front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		Front = glm::normalize(front);
		// also re-calculate the Right and Up vector
		Right = glm::normalize(glm::cross(Front, WorldUp)); // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		Up = glm::normalize(glm::cross(Right, Front));
	
		// 添加视口参数更新
		halfH = glm::tan(glm::radians(fov));
		halfW = halfH * ScreenRatio;
		LeftBottomCorner = Front - halfW * Right - halfH * Up;
		LoopNum = 0;
	}

};
#endif