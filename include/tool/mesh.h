#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <tool/shader.h>
#include <tool/OBJ_Loader.h>

#include <string>
#include <vector>

using namespace std;

// 添加数据转换适配器
inline glm::vec3 ToGlmVec3(const objl::Vector3& v) {
    return glm::vec3(v.X, v.Y, v.Z);
}

inline glm::vec2 ToGlmVec2(const objl::Vector2& v) {
    return glm::vec2(v.X, v.Y);
}

#ifndef BUFFER_GROMETRY
struct Vertex
{
	glm::vec3 Position;	 // 顶点属性
	glm::vec3 Normal;		 // 法线
	glm::vec2 TexCoords; // 纹理坐标

	// 切线空间属性
	glm::vec3 Tangent;
	glm::vec3 Bitangent;
};
#endif

struct Texture
{
	unsigned int id;
	string type;
	string path;
};

class Mesh
{
public:
	// mesh Data
	vector<Vertex> vertices;
	vector<unsigned int> indices;
	vector<Texture> textures;
	unsigned int VAO;

	Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures)
	{
		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;

		// now that we have all the required data, set the vertex buffers and its attribute pointers.
		setupMesh();
	}

    // 新增构造函数：从objl::Mesh转换
    Mesh(const objl::Mesh& objMesh, const std::string& basePath = "") 
    {
        // 转换顶点数据
        for (auto& v : objMesh.Vertices) {
            Vertex vert;
            vert.Position = ToGlmVec3(v.Position);
            vert.Normal = ToGlmVec3(v.Normal);
            vert.TexCoords = ToGlmVec2(v.TextureCoordinate);
            vert.Tangent = ToGlmVec3(v.Tangent);
            vert.Bitangent = ToGlmVec3(v.Bitangent);
            this->vertices.push_back(vert);
        }

        // 直接复制索引
        this->indices = objMesh.Indices;

        // 转换材质到纹理
        LoadTexturesFromMaterial(objMesh.MeshMaterial, basePath);

        setupMesh();
    }

	// render the mesh
	void Draw(Shader &shader)
	{
		// bind appropriate textures
		unsigned int diffuseNr = 1;
		unsigned int specularNr = 1;
		unsigned int normalNr = 1;
		unsigned int heightNr = 1;
		for (unsigned int i = 0; i < textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding
																				// retrieve texture number (the N in diffuse_textureN)
			string number;
			string name = textures[i].type;
			if (name == "texture_diffuse")
				number = std::to_string(diffuseNr++);
			else if (name == "texture_specular")
				number = std::to_string(specularNr++); // transfer unsigned int to stream
			else if (name == "texture_normal")
				number = std::to_string(normalNr++); // transfer unsigned int to stream
			else if (name == "texture_height")
				number = std::to_string(heightNr++); // transfer unsigned int to stream

			// now set the sampler to the correct texture unit
			glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);
			// and finally bind the texture
			glBindTexture(GL_TEXTURE_2D, textures[i].id);
		}

		// draw mesh
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// always good practice to set everything back to defaults once configured.
		glActiveTexture(GL_TEXTURE0);
	}

private:
	// render data
	unsigned int VBO, EBO;

	void setupMesh()
	{
		// create buffers/arrays
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);
		// load data into vertex buffers
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		// A great thing about structs is that their memory layout is sequential for all its items.
		// The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
		// again translates to 3/2 floats which translates to a byte array.
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		// set the vertex attribute pointers
		// vertex Positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
		// vertex normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Normal));
		// vertex texture coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, TexCoords));
		// vertex tangent
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Tangent));
		// vertex bitangent
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Bitangent));

		glBindVertexArray(0);
	}

    // 新增材质到纹理的转换方法
    void LoadTexturesFromMaterial(const objl::Material& mat, const std::string& basePath) {
        auto loadTex = [&](const std::string& path, const std::string& type) {
            if (!path.empty()) {
                Texture tex;
                tex.id = this->TextureFromFile(path, basePath); // 需要实现纹理加载函数
                tex.type = type;
                tex.path = path;
                textures.push_back(tex);
            }
        };

        loadTex(mat.map_Kd, "texture_diffuse");
        loadTex(mat.map_Ks, "texture_specular");
        loadTex(mat.map_bump, "texture_normal");
        loadTex(mat.map_Ka, "texture_ambient");
    }

	int TextureFromFile(const std::string& path, const std::string& directory) 
	{
		std::string filename = directory + '/' + path;
		
		unsigned int textureID;
		glGenTextures(1, &textureID);
		
		int width, height, nrComponents;
		unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
		if (data)
		{
			// ... 保持原有纹理加载逻辑 ...
			GLenum format = GL_RGB;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else
		{
			std::cout << "Texture failed to load at path: " << path << std::endl;
		}
		stbi_image_free(data);
		return textureID;
	}
};

#endif