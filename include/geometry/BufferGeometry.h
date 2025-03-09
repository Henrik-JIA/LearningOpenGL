#ifndef BUFFER_GEOMETRY
#define BUFFER_GEOMETRY

#define GLM_ENABLE_EXPERIMENTAL

#include <glad/glad.h>


#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <vector>
#include <iostream>

using namespace std;

const float PI = glm::pi<float>();

// mesh.h中也定义此属性
struct Vertex
{
    glm::vec3 Position;  // 顶点位置
    glm::vec3 Normal;    // 法线
    glm::vec2 TexCoords; // 纹理坐标
    glm::vec3 Color;     // 顶点颜色

    glm::vec3 Tangent;   // 切线
    glm::vec3 Bitangent; // 副切线
};

class BufferGeometry
{
public:
    vector<Vertex> vertices; // 顶点数据
    vector<unsigned int> indices; // 索引数据
    unsigned int VAO; // 顶点数组对象

    // 打印顶点数据
    void logParameters()
    {
        for (unsigned int i = 0; i < vertices.size(); i++)
        {
        cout << "-----------------" << endl;
        cout << "vertex ->> x: " << vertices[i].Position.x << ",y: " << vertices[i].Position.y << ",z: " << vertices[i].Position.z << endl;
        cout << "normal ->> x: " << vertices[i].Normal.x << ",y: " << vertices[i].Normal.y << ",z: " << vertices[i].Normal.z << endl;
        cout << "TexCoords ->> x: " << vertices[i].TexCoords.x << ",y: " << vertices[i].TexCoords.y << endl;
        cout << "-----------------" << endl;
        }
    }

    // 计算切线向量并添加到顶点属性中
    void computeTangents()
    {
    }

    // 释放资源
    void dispose()
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }

private:
    glm::mat4 matrix = glm::mat4(1.0f);

protected:
    unsigned int VBO, EBO; // 顶点缓冲对象和索引缓冲对象

    // 初始化缓冲区
    void setupBuffers()
    {
        // 创建顶点数组对象、顶点缓冲对象和索引缓冲对象
        glGenVertexArrays(1, &VAO); // 创建1个VAO顶点数组对象，ID存入VAO变量
        glGenBuffers(1, &VBO);      // 创建1个VBO顶点缓冲对象，ID存入VBO变量
        glGenBuffers(1, &EBO);      // 创建1个EBO索引缓冲对象，ID存入EBO变量

        // 绑定顶点数组对象
        // 开始记录顶点属性配置（VAO绑定后才会记录后续操作）
        glBindVertexArray(VAO); // 绑定VAO到当前上下文

        // 绑定VBO到顶点缓冲目标
        glBindBuffer(GL_ARRAY_BUFFER, VBO); 
        // 将CPU的vertices数组数据复制到GPU的VBO中
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_DYNAMIC_DRAW);

        // 绑定EBO到索引缓冲目标
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        // 将CPU的indices数组数据复制到GPU的EBO中
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // 设置顶点属性指针
        /* 
        以Position为例，参数说明：
            0：对应shader中的layout(location=0)
            3：每个顶点有3个分量（x,y,z）
            GL_FLOAT：数据类型为浮点数
            GL_FALSE：不需要标准化
            sizeof(Vertex)：步长（每个顶点数据的总字节长度）
            (void *)0：起始偏移量
        */
        // Position 位置
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
        glEnableVertexAttribArray(0); // 启用顶点属性位置0，0表示glsl中的layout(location=0)

        // Normal 法线
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Normal));
        glEnableVertexAttribArray(2);

        // TexCoords 纹理坐标   
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, TexCoords));
        glEnableVertexAttribArray(3);

        // Color 颜色
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Color));
        glEnableVertexAttribArray(1);

        // Tangent 切线
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Tangent));
        glEnableVertexAttribArray(4);

        // Bitangent 副切线
        glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Bitangent));
        glEnableVertexAttribArray(5);
        
        // 解绑顶点缓冲对象和顶点数组对象
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0); // 0表示解绑当前VAO
    }
};
#endif