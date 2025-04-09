// File: include/tool/ScreenFBO.h
#pragma once
#ifndef SCREEN_FBO_H
#define SCREEN_FBO_H

#include <glad/glad.h>

class ScreenFBO {
public:
	// 构造函数
	ScreenFBO(){ }

	// framebuffer配置
	unsigned int framebuffer; // 自定义帧缓冲
	// 颜色附件纹理
	unsigned int textureColorbuffer; // 颜色纹理附件
	// 深度和模板附件的renderbuffer object
	unsigned int rbo; // 渲染缓冲对象

	void configuration(int SCR_WIDTH, int SCR_HEIGHT) {
		// 1. 创建帧缓冲对象
		glGenFramebuffers(1, &framebuffer); // 创建帧缓冲
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer); // 绑定自定义帧缓冲
		
		// 2. 创建颜色附件（纹理）
		// 绑定颜色纹理
		glGenTextures(1, &textureColorbuffer); // 生成纹理
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer); // 绑定纹理
		// 设置纹理参数(大小与屏幕相同)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL); // 设置纹理数据
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // 设置纹理过滤，GL_LINEAR表示线性过滤，GL_TEXTURE_MIN_FILTER表示缩小过滤
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // 设置纹理过滤，GL_LINEAR表示线性过滤，GL_TEXTURE_MAG_FILTER表示放大过滤
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // 设置纹理环绕方式
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // 设置纹理环绕方式

		// 3. 将颜色纹理附加到当前绑定的帧缓冲对象
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);

		// 4. 创建渲染缓冲对象(用于深度和模板测试)
		glGenRenderbuffers(1, &rbo); // 生成渲染缓冲
		glBindRenderbuffer(GL_RENDERBUFFER, rbo); // 绑定渲染缓冲
		// 设置深度和模板缓冲
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT); // 设置渲染缓冲存储

		// 5. 将渲染缓冲附加到帧缓存对象的深度和模板附件上，渲染缓冲也是帧缓冲对象，区别在于帧缓冲可以进行采样，渲染缓冲不能进行采样
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // 将渲染缓冲对象附加到帧缓冲的深度和模板附件上

		// 6. 检查帧缓冲是否是完整
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			// 没有正确实现，则报错
			std::cout << "ERROR:Framebuffer is not complete!" << std::endl;
		}

		// 7. 解绑帧缓冲（将帧缓冲对象绑定到默认帧缓存上）绑定到默认FrameBuffer
		// unBind();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void Bind() {
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glDisable(GL_DEPTH_TEST);
	}

	void unBind() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void BindAsTexture() {
		// 作为第0个纹理
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
	}

	void Delete() {
		// 删除
		unBind();
		glDeleteFramebuffers(1, &framebuffer);
		glDeleteTextures(1, &textureColorbuffer);
	}

};

#endif // SCREEN_FBO_H