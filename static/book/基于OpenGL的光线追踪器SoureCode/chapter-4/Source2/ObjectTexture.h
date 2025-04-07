#pragma once
#ifndef __ObjectTexture_h__
#define __ObjectTexture_h__


#include "../glad/glad.h"
#include "../GLFW/glfw3.h"

#include "../Source/Mesh.h"
#include "../Source/Shader.h"
#include "../Source/BVHTree.h"

#include <cmath>
#include <vector>

class ObjectTexture {
public:
	GLuint ID_vTex;
	GLuint ID_fTex;
	int dataSize_v, meshFaceNum;

	void setTex(Shader &shader) {

		shader.setInt("meshVertexNum", dataSize_v);
		shader.setInt("meshFaceNum", meshFaceNum);

		glActiveTexture(GL_TEXTURE0 + 1);
		// and finally bind the texture
		glBindTexture(GL_TEXTURE_2D, ID_vTex);

		// ���������
		glActiveTexture(GL_TEXTURE0 + 2);
		// and finally bind the texture
		glBindTexture(GL_TEXTURE_2D, ID_fTex);

		shader.setInt("texMeshVertex", 1);
		shader.setInt("texMeshFaceIndex", 2);
	}

};


void getTexture(std::vector<Mesh> & data, Shader& shader, ObjectTexture& objTex, BVHTree& bvhTree, float Scale = 1.0f, float bias = 0.0f) {
	int dataSize_v = 0, dataSize_f = 0;
	for (int i = 0; i < data.size(); i++) {
		// �ۼ�ÿ��Mesh��size
		dataSize_v += data[i].vertices.size();
		dataSize_f += data[i].indices.size();
	}
	std::cout << "dataSize_t = " << dataSize_v << std::endl;
	std::cout << "dataSize_f = " << dataSize_f << std::endl;

	// �����ݷ��䵽����ά��
	// ������ʷ�ʽ��(i/vert_x,i%vert_x)
	// ע�������ʵ�ʳ�����Ҫ����8������3ά+������3ά+��������2ά����
	float vert_x_f = sqrtf(dataSize_v);
	int vert_x = ceilf(vert_x_f);
	int vert_y = ceilf((float)dataSize_v / (float)vert_x);
	std::cout << "vert_x = " << vert_x << " vert_y = " << vert_y << std::endl;

	float face_x_f = sqrtf(dataSize_f);
	int face_x = ceilf(face_x_f);
	int face_y = ceilf((float)dataSize_f / (float)face_x);
	std::cout << "face_x = " << face_x << " face_y = " << face_y << std::endl;

	// ÿ���������3��float�������꣬3��float��������2��float��uv����
	float *vertexArray = new float[(3 + 3 + 2) * (vert_x * vert_y)];
	float *faceArray = new float[(1) * (face_x * face_y)];
	int v_index = 0;
	int f_index = 0;
	for (int i = 0; i < data.size(); i++) {
		// ����ÿ��Mesh�Ķ���
		for (int j = 0;j < data[i].vertices.size(); j++) {
			vertexArray[v_index * (8) + 0] = Scale * data[i].vertices[j].Position.x;
			vertexArray[v_index * (8) + 1] = Scale * data[i].vertices[j].Position.y - bias;
			vertexArray[v_index * (8) + 2] = Scale * data[i].vertices[j].Position.z;

			vertexArray[v_index * (8) + 3] = data[i].vertices[j].Normal.x;
			vertexArray[v_index * (8) + 4] = data[i].vertices[j].Normal.y;
			vertexArray[v_index * (8) + 5] = data[i].vertices[j].Normal.z;

			vertexArray[v_index * (8) + 6] = data[i].vertices[j].TexCoords.x;
			vertexArray[v_index * (8) + 7] = data[i].vertices[j].TexCoords.y;

			v_index++;
		}
		// ������Ƭ��������
		for (int j = 0; j < data[i].indices.size(); j++) {
			faceArray[f_index] = data[i].indices[j];
			f_index++;
		}
	}

	// �����θ��� = dataSize_v
	objTex.dataSize_v = dataSize_v;
	// �����θ��� = dataSize_f / 3
	objTex.meshFaceNum = dataSize_f / 3;
	
	// ����ÿ��������
	std::vector<std::shared_ptr<Triangle>> primitives;
	for (int i = 0; i < objTex.meshFaceNum; i++) {
		std::shared_ptr<Triangle> tri = std::make_shared<Triangle>();
		tri->v0 = glm::vec3(vertexArray[(int)faceArray[i * 3 + 0] * 8 + 0], vertexArray[(int)faceArray[i * 3 + 0] * 8 + 1], vertexArray[(int)faceArray[i * 3 + 0] * 8 + 2]);
		tri->v1 = glm::vec3(vertexArray[(int)faceArray[i * 3 + 1] * 8 + 0], vertexArray[(int)faceArray[i * 3 + 1] * 8 + 1], vertexArray[(int)faceArray[i * 3 + 1] * 8 + 2]);
		tri->v2 = glm::vec3(vertexArray[(int)faceArray[i * 3 + 2] * 8 + 0], vertexArray[(int)faceArray[i * 3 + 2] * 8 + 1], vertexArray[(int)faceArray[i * 3 + 2] * 8 + 2]);
		primitives.push_back(tri);
	}
	std::cout << "primitives.size():" << primitives.size() << std::endl;

	// ����BVH��
	bvhTree.BVHBuildTree(primitives);

	// �󶨵�������
	shader.use();

	glGenTextures(1, &objTex.ID_vTex);
	glBindTexture(GL_TEXTURE_2D, objTex.ID_vTex);//dataSize_v * 8
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, vert_x * 8, vert_y, 0, GL_RED, GL_FLOAT, vertexArray);
	// ����ڲ�ֵ
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// ������
	shader.setInt("texMeshVertex", 1);

	glGenTextures(1, &objTex.ID_fTex);
	glBindTexture(GL_TEXTURE_2D, objTex.ID_fTex); //dataSize_f
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, face_x, face_y, 0, GL_RED, GL_FLOAT, faceArray);
	// ����ڲ�ֵ
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// ������
	shader.setInt("texMeshFaceIndex", 2);

	// ɾ������
	delete[] vertexArray;
	delete[] faceArray;

}





#endif






