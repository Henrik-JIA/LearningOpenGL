#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>       // 添加strerror支持
#include <stdexcept>     // 添加标准异常支持

class Shader {
public:
    unsigned int ID;

    // 函数1：支持直接传入着色器源码
    static Shader FromSource(const char* vertexSource, const char* fragmentSource, const char* geometrySource = nullptr) {
        Shader shader;
        shader.compileFromSource(vertexSource, fragmentSource, geometrySource);
        return shader;
    }

    // 函数2：支持从文件路径加载着色器
    static Shader FromFile(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr) {
        Shader shader;
        std::string vertexCode = shader.readFile(vertexPath);
        std::string fragmentCode = shader.readFile(fragmentPath);
        std::string geometryCode;
        
        if(geometryPath != nullptr) {
            geometryCode = shader.readFile(geometryPath);
        }

        shader.compileFromSource(vertexCode.c_str(), fragmentCode.c_str(), 
                               geometryPath ? geometryCode.c_str() : nullptr);
        return shader;
    }


    ~Shader() {
        glDeleteProgram(ID);
    }

    void use() { 
        glUseProgram(ID);
    }

    // Uniform 设置方法
    // 统一获取uniform位置（保持原有实现）
    GLint getUniformLocation(const std::string &name) const {
        return glGetUniformLocation(ID, name.c_str());
    }

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
    // 将构造函数设为私有
    Shader() = default;

    // 统一编译方法
    void compileFromSource(const char* vertexSource, const char* fragmentSource, const char* geometrySource = nullptr) {
        unsigned int vertex = compileShader(GL_VERTEX_SHADER, vertexSource);
        unsigned int fragment = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
        unsigned int geometry = 0;

        if(geometrySource != nullptr) {
            geometry = compileShader(GL_GEOMETRY_SHADER, geometrySource);
        }

        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        if(geometrySource != nullptr) glAttachShader(ID, geometry);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if(geometrySource != nullptr) glDeleteShader(geometry);
    }

    // 文件读取方法
    std::string readFile(const char* path) {
        std::ifstream file;
        std::stringstream buffer;
        
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            file.open(path, std::ios::binary);
            buffer << file.rdbuf();
            file.close();
            return buffer.str();
        }
        catch (std::ifstream::failure& e) {
            if(file.is_open()) file.close();
            throw std::runtime_error(
                std::string("SHADER::FILE_READ_FAILURE\n") +
                "Path: " + path + "\n" +
                "Error: " + strerror(errno)
            );
        }
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
                throw std::runtime_error(
                    "SHADER_COMPILATION_ERROR: " + type + "\n" + infoLog
                );
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                throw std::runtime_error(
                    "PROGRAM_LINKING_ERROR: " + type + "\n" + infoLog
                );
            }
        }
    }
};
#endif