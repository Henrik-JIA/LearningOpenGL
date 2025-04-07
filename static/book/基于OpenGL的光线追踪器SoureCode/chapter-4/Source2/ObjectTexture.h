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

		// 激活并绑定纹理
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
		// 累加每个Mesh的size
		dataSize_v += data[i].vertices.size();
		dataSize_f += data[i].indices.size();
	}
	std::cout << "dataSize_t = " << dataSize_v << std::endl;
	std::cout << "dataSize_f = " << dataSize_f << std::endl;

	// 把数据分配到两个维度
	// 坐标访问方式：(i/vert_x,i%vert_x)
	// 注意横坐标实际长度需要乘以8（顶点3维+法向量3维+纹理坐标2维）。
	float vert_x_f = sqrtf(dataSize_v);
	int vert_x = ceilf(vert_x_f);
	int vert_y = ceilf((float)dataSize_v / (float)vert_x);
	std::cout << "vert_x = " << vert_x << " vert_y = " << vert_y << std::endl;

	float face_x_f = sqrtf(dataSize_f);
	int face_x = ceilf(face_x_f);
	int face_y = ceilf((float)dataSize_f / (float)face_x);
	std::cout << "face_x = " << face_x << " face_y = " << face_y << std::endl;

	// 每个顶点包含3个float顶点坐标，3个float法向量，2个float的uv坐标
	float *vertexArray = new float[(3 + 3 + 2) * (vert_x * vert_y)];
	float *faceArray = new float[(1) * (face_x * face_y)];
	int v_index = 0;
	int f_index = 0;
	for (int i = 0; i < data.size(); i++) {
		// 处理每个Mesh的顶点
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
		// 处理面片顶点索引
		for (int j = 0; j < data[i].indices.size(); j++) {
			faceArray[f_index] = data[i].indices[j];
			f_index++;
		}
	}

	// 三角形个数 = dataSize_v
	objTex.dataSize_v = dataSize_v;
	// 三角形个数 = dataSize_f / 3
	objTex.meshFaceNum = dataSize_f / 3;
	
	// 生成每个三角形
	std::vector<std::shared_ptr<Triangle>> primitives;
	for (int i = 0; i < objTex.meshFaceNum; i++) {
		std::shared_ptr<Triangle> tri = std::make_shared<Triangle>();
		tri->v0 = glm::vec3(vertexArray[(int)faceArray[i * 3 + 0] * 8 + 0], vertexArray[(int)faceArray[i * 3 + 0] * 8 + 1], vertexArray[(int)faceArray[i * 3 + 0] * 8 + 2]);
		tri->v1 = glm::vec3(vertexArray[(int)faceArray[i * 3 + 1] * 8 + 0], vertexArray[(int)faceArray[i * 3 + 1] * 8 + 1], vertexArray[(int)faceArray[i * 3 + 1] * 8 + 2]);
		tri->v2 = glm::vec3(vertexArray[(int)faceArray[i * 3 + 2] * 8 + 0], vertexArray[(int)faceArray[i * 3 + 2] * 8 + 1], vertexArray[(int)faceArray[i * 3 + 2] * 8 + 2]);
		primitives.push_back(tri);
	}
	std::cout << "primitives.size():" << primitives.size() << std::endl;

	// 构建BVH树
	bvhTree.BVHBuildTree(primitives);

	// 绑定到纹理中
	shader.use();

	glGenTextures(1, &objTex.ID_vTex);
	glBindTexture(GL_TEXTURE_2D, objTex.ID_vTex);//dataSize_v * 8
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, vert_x * 8, vert_y, 0, GL_RED, GL_FLOAT, vertexArray);
	// 最近邻插值
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// 绑定纹理
	shader.setInt("texMeshVertex", 1);

	glGenTextures(1, &objTex.ID_fTex);
	glBindTexture(GL_TEXTURE_2D, objTex.ID_fTex); //dataSize_f
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, face_x, face_y, 0, GL_RED, GL_FLOAT, faceArray);
	// 最近邻插值
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// 绑定纹理
	shader.setInt("texMeshFaceIndex", 2);

	// 删除数组
	delete[] vertexArray;
	delete[] faceArray;

}





#endif






