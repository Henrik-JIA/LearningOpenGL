## 帧缓冲实现说明

### main.cpp 存在问题
- 自定义帧缓冲渲染后屏幕显示异常（可能显示黑屏或内容不更新）
- 问题原因：
  未找到

### main2.cpp 正确实现
- 正常使用自定义帧缓冲进行离屏渲染
- 关键修正点：
  1. 在窗口大小回调中正确更新附件尺寸：
```cpp:src/19_Framebuffers/main2.cpp (lines 997-1013)
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // ... existing code ...
    
    // 更新颜色附件纹理尺寸
    glBindTexture(GL_TEXTURE_2D, texColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    
    // 更新渲染缓冲对象尺寸
    glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
}
```
  2. 使用正确的深度测试状态管理
  3. 确保帧缓冲完整性检查
  4. 正确绑定/解绑帧缓冲对象

建议直接使用 main2.cpp 作为帧缓冲实现的参考版本。