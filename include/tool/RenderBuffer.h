#pragma once
#ifndef RENDER_BUFFER_H
#define RENDER_BUFFER_H

#include <tool/ScreenFBO.h>

#include <iostream>

using namespace std;

// 双缓冲帧缓存管理机制
class RenderBuffer {
public:
	// 初始化，创建了两个帧缓冲对象，创建两个相同尺寸的FBO（帧缓冲对象）
	void Init(int SCR_WIDTH, int SCR_HEIGHT) {
		fbo[0].configuration(SCR_WIDTH, SCR_HEIGHT);
		fbo[1].configuration(SCR_WIDTH, SCR_HEIGHT);
		currentIndex = 0;
	}

	// 设置当前帧的帧缓冲对象
	// fbo[0]为渲染到当前帧的内容
	// 实现缓冲切换（当前帧渲染，历史帧作为输入纹理） 
	void setCurrentBuffer(int LoopNum) {
		int histIndex = LoopNum % 2; // 获取历史帧的索引
		int curIndex = (histIndex == 0 ? 1 : 0); // 获取当前帧的索引
		
		std::cout << "Frame " << LoopNum << ": "
				<< "Writing to FBO[" << curIndex << "](ID:" << fbo[curIndex].framebuffer << "), "
				<< "Reading from FBO[" << histIndex << "](ID:" << fbo[histIndex].framebuffer << ")\n";
		
		fbo[curIndex].Bind(); // 绑定当前帧的帧缓冲对象
		fbo[histIndex].BindAsTexture(); // 绑定历史帧的纹理
	}
	
	// 将当前活跃FBO绑定为纹理
	void setCurrentAsTexture(int LoopNum) {
		int histIndex = LoopNum % 2; // 获取历史帧的索引
		int curIndex = (histIndex == 0 ? 1 : 0); // 获取当前帧的索引
		fbo[curIndex].BindAsTexture(); // 绑定当前帧的纹理
	}

	// 解绑帧缓冲
	void unBind() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0); // 直接解绑到默认帧缓冲
	}

	// 删除帧缓冲对象
	void Delete() {
		fbo[0].Delete();
		fbo[1].Delete();
	}
private:
	// 用于渲染当前帧的索引
	int currentIndex;
	ScreenFBO fbo[2]; // 创建了2个ScreenFBO类的实例，在栈内存中连续分配了2个ScreenFBO对象
};

#endif // RENDER_BUFFER_H




