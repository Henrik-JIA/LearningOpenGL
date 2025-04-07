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
	GLuint ID_meshTex;
	GLuint ID_bvhNodeTex;
	int meshNum, meshFaceNum;

	void setTex(Shader &shader) {

		shader.setInt("meshNum", meshNum);
		shader.setInt("bvhNodeNum", meshFaceNum);

		glActiveTexture(GL_TEXTURE0 + 1);
		// and finally bind the texture
		glBindTexture(GL_TEXTURE_2D, ID_meshTex);

		// 激活并绑定纹理
		glActiveTexture(GL_TEXTURE0 + 2);
		// and finally bind the texture
		glBindTexture(GL_TEXTURE_2D, ID_bvhNodeTex);

		shader.setInt("texMesh", 1);
		shader.setInt("texBvhNode", 2);
	}

};


void getTexture(std::vector<Mesh> & data, Shader& shader, ObjectTexture& objTex, BVHTree& bvhTree, float Scale = 1.0f, glm::vec3 bias = glm::vec3(0.0f)) {
	int dataSize_v = 0, dataSize_f = 0;
	for (int i = 0; i < data.size(); i++) {
		// 累加每个Mesh的size
		dataSize_v += data[i].vertices.size();
		dataSize_f += data[i].indices.size();
	}
	std::cout << "dataSize_t = " << dataSize_v << std::endl;
	std::cout << "dataSize_f = " << dataSize_f << std::endl;

	// 三角形个数
	objTex.meshNum = dataSize_f / 3;
	// 三角形个数 = dataSize_f / 3
	objTex.meshFaceNum = dataSize_f / 3;

	// 生成每个三角形
	std::vector<std::shared_ptr<Triangle>> primitives;
	for (int i = 0; i < data.size(); i++) {
		for (int j = 0; j < data[i].indices.size() / 3; j++) {
			std::shared_ptr<Triangle> tri = std::make_shared<Triangle>();
			tri->v0 = Scale * data[i].vertices[data[i].indices[j * 3 + 0]].Position + bias;
			tri->v1 = Scale * data[i].vertices[data[i].indices[j * 3 + 1]].Position + bias;
			tri->v2 = Scale * data[i].vertices[data[i].indices[j * 3 + 2]].Position + bias;
			primitives.push_back(tri);
		}
	}
	std::cout << "primitives.size():" << primitives.size() << std::endl;

	// 构建BVH树
	bvhTree.BVHBuildTree(primitives);

	// 绑定到纹理中
	shader.use();

	glGenTextures(1, &objTex.ID_meshTex);
	glBindTexture(GL_TEXTURE_2D, objTex.ID_meshTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, bvhTree.meshNumX, bvhTree.meshNumY, 0, GL_RED, GL_FLOAT, bvhTree.MeshArray);
	// 最近邻插值
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// 绑定纹理
	shader.setInt("texMesh", 1);

	glGenTextures(1, &objTex.ID_bvhNodeTex);
	glBindTexture(GL_TEXTURE_2D, objTex.ID_bvhNodeTex); //dataSize_f
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, bvhTree.nodeNumX, bvhTree.nodeNumY, 0, GL_RED, GL_FLOAT, bvhTree.NodeArray);
	// 最近邻插值
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// 绑定纹理
	shader.setInt("texBvhNode", 2);

	// 删除数组
	// 等测试完再删除
}





#endif






