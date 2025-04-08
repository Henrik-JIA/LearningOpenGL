#pragma once
#ifndef __RT_Screen_h__
#define __RT_Screen_h__

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// 屏幕的顶点坐标和纹理坐标
const float ScreenVertices[] = {
	//位置坐标(x,y,z)     //纹理坐标

	//三角形1
	-1.0f, 1.0f, 0.0f, 0.0f, 1.0f, //左上角 
	-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, //左下角
	1.0f, -1.0f, 0.0f, 1.0f, 0.0f, //右下角

	//三角形2
	-1.0f,  1.0f, 0.0f, 0.0f, 1.0f, //左上角
	1.0f, -1.0f, 0.0f, 1.0f, 0.0f, //右下角
	1.0f,  1.0f, 0.0f, 1.0f, 1.0f  //右上角
};

// 屏幕类，用于创建一个和当前窗口大小相同的屏幕，这个屏幕的顶点坐标和纹理坐标已经确定，只需要绑定VAO并开始绘制即可
class RT_Screen {
public:
	// 初始化屏幕的VAO和VBO
	void InitScreenBind() {
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(ScreenVertices), ScreenVertices, GL_STATIC_DRAW);
		// 位置
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		// 纹理
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// Unbind VAO
		glBindVertexArray(0);
	}
	// 绘制屏幕
	void DrawScreen() {
		// 绑定VAO并开始绘制
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
private:
	unsigned int VBO, VAO;

};












#endif







