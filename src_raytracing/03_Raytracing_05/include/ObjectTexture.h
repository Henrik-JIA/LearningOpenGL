#pragma once
#ifndef __ObjectTexture_h__
#define __ObjectTexture_h__


#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <tool/Mesh.h>
#include <tool/Shader.h>
#include <tool/BVHTree.h>

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


void getTexture(const std::vector<Mesh> & data, 
				Shader& shader, 
				ObjectTexture& objTex, 
				std::vector<std::shared_ptr<Triangle>>& primitives, 
				BVHTree& bvhTree, 
				float Scale = 1.0f, 
				glm::vec3 bias = glm::vec3(0.0f)) 
{
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

}


void getTextureWithTransform(const std::vector<Mesh> & data, 
							Shader& shader, 
							ObjectTexture& objTex, 
							std::vector<std::shared_ptr<Triangle>>& primitives, 
							BVHTree& bvhTree, 
							glm::vec3 position = glm::vec3(0.0f),
							float scale = 1.0f,
							float rotateAngle = 0.0f,
							glm::vec3 rotateAxis = glm::vec3(0.0f, 1.0f, 0.0f), // 默认绕Y轴旋转
							glm::vec3 albedo = glm::vec3(0.8f),
							int materialType = 0
							) 
{
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
	
	// 构建模型矩阵（顺序：旋转 -> 缩放 -> 平移）
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    modelMatrix = glm::scale(modelMatrix, glm::vec3(scale));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotateAngle), rotateAxis);

    for (int i = 0; i < data.size(); i++) {
        for (int j = 0; j < data[i].indices.size() / 3; j++) {
            std::shared_ptr<Triangle> tri = std::make_shared<Triangle>();
            
            // 应用完整模型变换
            glm::vec4 v0 = modelMatrix * glm::vec4(data[i].vertices[data[i].indices[j*3+0]].Position, 1.0f);
            glm::vec4 v1 = modelMatrix * glm::vec4(data[i].vertices[data[i].indices[j*3+1]].Position, 1.0f);
            glm::vec4 v2 = modelMatrix * glm::vec4(data[i].vertices[data[i].indices[j*3+2]].Position, 1.0f);
            
            tri->v0 = glm::vec3(v0);
            tri->v1 = glm::vec3(v1);
            tri->v2 = glm::vec3(v2);
	
			glm::vec4 n0 = modelMatrix * glm::vec4(data[i].vertices[data[i].indices[j*3+0]].Normal, 0.0f);
			glm::vec4 n1 = modelMatrix * glm::vec4(data[i].vertices[data[i].indices[j*3+1]].Normal, 0.0f);
			glm::vec4 n2 = modelMatrix * glm::vec4(data[i].vertices[data[i].indices[j*3+2]].Normal, 0.0f);
			
			tri->n0 = glm::vec3(n0);
			tri->n1 = glm::vec3(n1);
			tri->n2 = glm::vec3(n2);

			glm::vec2 u0 = glm::vec2(data[i].vertices[data[i].indices[j*3+0]].TexCoords);
			glm::vec2 u1 = glm::vec2(data[i].vertices[data[i].indices[j*3+1]].TexCoords);
			glm::vec2 u2 = glm::vec2(data[i].vertices[data[i].indices[j*3+2]].TexCoords);	
			
			tri->u0 = u0;
			tri->u1 = u1;
			tri->u2 = u2;

			tri->albedo = albedo;

			tri->materialType = materialType;

            primitives.push_back(tri);
        }
    }
	std::cout << "primitives.size():" << primitives.size() << std::endl;

}

void generateTextures(ObjectTexture& objTex, BVHTree& bvhTree, Shader& shader) {
	
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



