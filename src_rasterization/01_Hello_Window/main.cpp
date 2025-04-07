#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// 检查用户是否按下了返回键(Esc)
void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        std::cout<< "Esc is pressed" << std::endl;
        glfwSetWindowShouldClose(window, true);
    }
        
}

int main(){
    // 1.初始化GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);  // 设置OpenGL主版本号为3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);  // 设置OpenGL次版本号为3
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 使用核心模式
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 向前兼容模式（启用后会移除所有被标记为已废弃的OpenGL功能）

    // 2.创建一个GLFW窗口对象，设置为当前线程的主上下文
    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout<< "Failed to create GLFW window!" << std::endl;
        // 窗口创建失败，则释放所有glfw资源
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // 3.GLAD加载函数指针，调用任何OpenGL的函数之前我们需要初始化GLAD
    // 建议：GLAD的初始化必须放在创建OpenGL上下文（glfwMakeContextCurrent）之后，但在调用任何OpenGL函数之前。
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // 4.创建视口，使用OpenGL函数
    // 可以将视口的维度设置为比GLFW的维度小，
    // 这样子之后所有的OpenGL渲染将会在一个更小的窗口中显示，
    // 这样子的话我们也可以将一些其它元素显示在OpenGL视口之外。
    glViewport(0, 0, 800, 600);

    // 5.创建回调函数framebuffer_size_callback，以实现视口随着窗口调整而调整。
    // 需要注册这个framebuffer_size_callback函数（监听），告诉GLFW我们希望每当窗口调整大小的时候调用这个函数
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 6.渲染循环(Render Loop)，它能在我们让GLFW退出前一直保持运行。
    // glfwWindowShouldClose函数：检查一次GLFW是否被要求退出。
    // 自定义processInput函数：来让所有的输入代码保持整洁。
    // glfwPollEvents函数：检查有没有触发事件（比如键盘输入、鼠标移动等）、更新窗口状态，并调用对应的回调函数（可以通过回调方法手动设置）。
    // glfwSwapBuffers函数会交换颜色缓冲（它是一个储存着GLFW窗口每一个像素颜色值的大缓冲），它在这一迭代中被用来绘制，并且将会作为输出显示在屏幕上。
    // 应用双缓冲渲染窗口应用程序。前缓冲保存着最终输出的图像，它会在屏幕上显示；而所有的的渲染指令都会在后缓冲上绘制。当所有的渲染指令执行完毕后，我们交换(Swap)前缓冲和后缓冲，这样图像就立即呈显出来，之前提到的不真实感就消除了。
    while(!glfwWindowShouldClose(window))
    {
        // （1）输入
        processInput(window);

        // （2）渲染指令：
        // 调用了glClearColor来设置清空屏幕所用的颜色
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        // 可以通过调用glClear函数来清空屏幕的颜色缓冲，它接受一个缓冲位(Buffer Bit)来指定要清空的缓冲，
        // 可能的缓冲位有GL_COLOR_BUFFER_BIT，GL_DEPTH_BUFFER_BIT和GL_STENCIL_BUFFER_BIT。由于现在我们只关心颜色值，所以我们只清空颜色缓冲。
        // 调用glClear函数，清除颜色缓冲之后，整个颜色缓冲都会被填充为glClearColor里所设置的颜色
        glClear(GL_COLOR_BUFFER_BIT);

        // （3）检查并调用事件，交换缓冲
        glfwSwapBuffers(window);
        glfwPollEvents();    
    }

    // 7.当渲染循环结束后我们需要正确释放/删除之前的分配的所有资源
    glfwTerminate();
    
    return 0;
}


