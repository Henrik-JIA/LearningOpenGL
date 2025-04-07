#pragma once
#ifndef __RT_Screen_h__
#define __RT_Screen_h__

#include "../glad/glad.h"
#include "../GLFW/glfw3.h"

const float ScreenVertices[] = {
	//λ������(x,y)     //��������

	//������1
	-1.0f, 1.0f,   0.0f, 1.0f, //���Ͻ� 
	-1.0f, -1.0f,  0.0f, 0.0f, //���½�
	1.0f, -1.0f,   1.0f, 0.0f, //���½�

	//������2
	-1.0f,  1.0f,  0.0f, 1.0f, //���Ͻ�
	1.0f, -1.0f,   1.0f, 0.0f, //���½�
	1.0f,  1.0f,   1.0f, 1.0f  //���Ͻ�
};

class RT_Screen {
public:
	void InitScreenBind() {
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(ScreenVertices), ScreenVertices, GL_STATIC_DRAW);
		// λ��
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		// ����
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// Unbind VAO
		glBindVertexArray(0);
	}
	void DrawScreen() {
		// ��VAO����ʼ����
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
private:
	unsigned int VBO, VAO;

};












#endif







