// File: include/tool/ScreenFBO.h
#pragma once
#ifndef SCREEN_FBO_H
#define SCREEN_FBO_H

#include <glad/glad.h>

class ScreenFBO {
public:
	ScreenFBO(){ }
	// framebuffer配置
	unsigned int framebuffer;
	// 颜色附件纹理
	unsigned int textureColorbuffer;
	// 深度和模板附件的renderbuffer object
	unsigned int rbo;
	void configuration(int SCR_WIDTH, int SCR_HEIGHT) {
		glGenFramebuffers(1, &framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		// 绑定颜色纹理
		glGenTextures(1, &textureColorbuffer);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			// 没有正确实现，则报错
		}
		// 绑定到默认FrameBuffer
		unBind();
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