#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
public:
    unsigned int ID;

    // 构造函数支持几何着色器（可选）
    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr) {
        // 1. 读取着色器代码
        std::string vertexCode = readFile(vertexPath);
        std::string fragmentCode = readFile(fragmentPath);
        std::string geometryCode;
        
        if(geometryPath != nullptr) {
            geometryCode = readFile(geometryPath);
        }

        // 2. 编译着色器
        unsigned int vertex = compileShader(GL_VERTEX_SHADER, vertexCode.c_str());
        unsigned int fragment = compileShader(GL_FRAGMENT_SHADER, fragmentCode.c_str());
        unsigned int geometry;
        
        if(geometryPath != nullptr) {
            geometry = compileShader(GL_GEOMETRY_SHADER, geometryCode.c_str());
        }

        // 3. 创建着色器程序
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        if(geometryPath != nullptr) glAttachShader(ID, geometry);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        // 4. 清理资源
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if(geometryPath != nullptr) glDeleteShader(geometry);
    }

    ~Shader() {
        glDeleteProgram(ID);
    }

    void use() { 
        glUseProgram(ID);
    }

    // Uniform 设置方法
    void setBool(const std::string &name, bool value) const {
        glUniform1i(getUniformLocation(name), (int)value);
    }
    
    void setInt(const std::string &name, int value) const {
        glUniform1i(getUniformLocation(name), value);
    }
    
    void setFloat(const std::string &name, float value) const {
        glUniform1f(getUniformLocation(name), value);
    }

    void setVec2(const std::string &name, const glm::vec2 &value) const {
        glUniform2fv(getUniformLocation(name), 1, &value[0]);
    }
    
    void setVec2(const std::string &name, float x, float y) const {
        glUniform2f(getUniformLocation(name), x, y);
    }

    void setVec3(const std::string &name, const glm::vec3 &value) const {
        glUniform3fv(getUniformLocation(name), 1, &value[0]);
    }
    
    void setVec3(const std::string &name, float x, float y, float z) const {
        glUniform3f(getUniformLocation(name), x, y, z);
    }

    void setVec4(const std::string &name, const glm::vec4 &value) const {
        glUniform4fv(getUniformLocation(name), 1, &value[0]);
    }
    
    void setVec4(const std::string &name, float x, float y, float z, float w) const {
        glUniform4f(getUniformLocation(name), x, y, z, w);
    }

    void setMat2(const std::string &name, const glm::mat2 &mat) const {
        glUniformMatrix2fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
    }

    void setMat3(const std::string &name, const glm::mat3 &mat) const {
        glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
    }

    void setMat4(const std::string &name, const glm::mat4 &mat) const {
        glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
    }
    
    // 新增的设置方法，接收uniform位置作为参数
    void setBool(GLint location, bool value) const {
        glUniform1i(location, (int)value);
    }
    
    void setInt(GLint location, int value) const {
        glUniform1i(location, value);
    }
    
    void setFloat(GLint location, float value) const {
        glUniform1f(location, value);
    }

    void setVec2(GLint location, const glm::vec2 &value) const {
        glUniform2fv(location, 1, &value[0]);
    }
    
    void setVec2(GLint location, float x, float y) const {
        glUniform2f(location, x, y);
    }

    void setVec3(GLint location, const glm::vec3 &value) const {
        glUniform3fv(location, 1, &value[0]);
    }
    
    void setVec3(GLint location, float x, float y, float z) const {
        glUniform3f(location, x, y, z);
    }

    void setVec4(GLint location, const glm::vec4 &value) const {
        glUniform4fv(location, 1, &value[0]);
    }
    
    void setVec4(GLint location, float x, float y, float z, float w) const {
        glUniform4f(location, x, y, z, w);
    }

    void setMat2(GLint location, const glm::mat2 &mat) const {
        glUniformMatrix2fv(location, 1, GL_FALSE, &mat[0][0]);
    }

    void setMat3(GLint location, const glm::mat3 &mat) const {
        glUniformMatrix3fv(location, 1, GL_FALSE, &mat[0][0]);
    }

    void setMat4(GLint location, const glm::mat4 &mat) const {
        glUniformMatrix4fv(location, 1, GL_FALSE, &mat[0][0]);
    }

private:
    // 统一获取uniform位置
    GLint getUniformLocation(const std::string &name) const {
        return glGetUniformLocation(ID, name.c_str());
    }

    // 文件读取
    std::string readFile(const char* path) {
        std::string code;
        std::ifstream file;
        
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            file.open(path);
            std::stringstream stream;
            stream << file.rdbuf();
            file.close();
            code = stream.str();
        }
        catch (std::ifstream::failure& e) {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << path 
                      << "\nError details: " << e.what() << std::endl;
        }
        return code;
    }

    // 编译着色器
    unsigned int compileShader(GLenum type, const char* code) {
        unsigned int shader = glCreateShader(type);
        glShaderSource(shader, 1, &code, NULL);
        glCompileShader(shader);
        checkCompileErrors(shader, 
            (type == GL_VERTEX_SHADER) ? "VERTEX" : 
            (type == GL_FRAGMENT_SHADER) ? "FRAGMENT" : "GEOMETRY");
        return shader;
    }

    // 错误检查
    void checkCompileErrors(unsigned int shader, std::string type) {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type 
                          << "\n" << infoLog 
                          << "\n---------------------------------------------------\n";
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type 
                          << "\n" << infoLog 
                          << "\n---------------------------------------------------\n";
            }
        }
    }
};
#endif