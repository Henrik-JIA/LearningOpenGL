#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

// 着色器代码
// 顶点着色器
const char *vertexShaderSource =    "#version 330 core\n"
                                    "layout (location = 0) in vec3 aPos;\n"
                                    "void main()\n"
                                    "{\n"
                                    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0f);\n"
                                    "   gl_PointSize = 10.0f;\n"
                                    "}\0";
// 片段着色器
const char *fragmentShaderSource =  "#version 330 core\n"
                                    "out vec4 FragColor;\n"
                                    "void main()\n"
                                    "{\n"
                                    "    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
                                    "}\0"; 

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

    // 顶点数据添加到显存空间：
    // 复制顶点数组到缓冲中供OpenGL使用。 
    // 6.定义顶点数组和顶点索引
    float vertices[] = {
        0.5f, 0.5f, 0.0f,   // 右上角
        0.5f, -0.5f, 0.0f,  // 右下角
        -0.5f, -0.5f, 0.0f, // 左下角
        -0.5f, 0.5f, 0.0f   // 左上角
    };
    unsigned int indices[] = {
        // 注意索引从0开始! 
        // 此例的索引(0,1,2,3)就是顶点数组vertices的下标，
        // 这样可以由下标代表顶点组合成矩形
        0, 1, 3, // 第一个三角形
        1, 2, 3  // 第二个三角形
    };
    // 7.创建VBO顶点缓冲对象
    unsigned int VBO;
    glGenBuffers(1, &VBO); // 这里需要传入VBO的引用
    // 8.创建VAO顶点数组对象，VAO的绑定必须在VBO和属性设置之前，才能正确捕获这些状态配置。
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    //  9.绑定VAO
    glBindVertexArray(VAO);
    // 10.顶点缓冲对象VBO与目标缓冲类型GL_ARRAY_BUFFER绑定
    // 目标缓冲的类型：顶点缓冲对象当前绑定到GL_ARRAY_BUFFER目标上
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // 11.定义的顶点数据vertices传递到顶点缓冲区
    // 把顶点数组复制到缓冲中供OpenGL使用。
    // 现在我们已经把顶点数据储存在显卡的内存中，用VBO这个顶点缓冲对象管理。
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // 设置顶点属性指针：
    // 10.设置顶点属性指针
    // 使用glVertexAttribPointer函数：告诉OpenGL该如何解析顶点数据（应用到逐个顶点属性上）
    // 使用glEnableVertexAttribArray函数：以顶点属性位置值作为参数，启用顶点属性；顶点属性默认是禁用的。
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 11.创建索引缓冲对象
    unsigned int EBO;
    glGenBuffers(1, &EBO);
    // 12.顶点索引缓冲对象EBO与目标缓冲类型GL_ELEMENT_ARRAY_BUFFER绑定
    // 目标缓冲的类型：引缓冲对象当前绑定到GL_ELEMENT_ARRAY_BUFFER目标上
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // 13.定义的索引indices传递到索引缓冲区
    // 把indices数组复制到缓冲中供OpenGL使用。
    // 现在我们已经把indices索引储存在显卡的内存中，用EBO这个索引缓冲对象管理。
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    // 14.解绑VAO（注意VAO一定是在VBO和EBO完成后最后解绑）
    glBindVertexArray(0);

    // 顶点着色器与片段着色器：
    // 10.创建顶点着色器：
    // 首先创建一个着色器对象，用glCreateShader创建这个着色器。
    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    // 11.把这个着色器源码&vertexShaderSource附加到着色器对象vertexShader上，然后编译它。
    // glShaderSource函数把要编译的着色器对象作为第一个参数。
    // 第二参数指定了传递的源码字符串数量，这里只有一个。
    // 第三个参数是顶点着色器真正的源码，第四个参数我们先设置为NULL。
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // 12.检测是否编译成功
    int vertexSuccess;
    char vertexInfoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vertexSuccess);
    if(!vertexSuccess)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, vertexInfoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << vertexInfoLog << std::endl;
    }
    // 13.创建片段着色器：
    // 首先创建一个着色器对象，用glCreateShader创建这个着色器。
    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    // 14.把这个着色器源码&fragmentShaderSource附加到着色器对象fragmentShader上，然后编译它。
    // glShaderSource函数把要编译的着色器对象作为第一个参数。
    // 第二参数指定了传递的源码字符串数量，这里只有一个。
    // 第三个参数是片段着色器真正的源码，第四个参数我们先设置为NULL。
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // 15.检测是否编译成功
    int fragmentSuccess;
    char fragmentInfoLog[512];
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fragmentSuccess);
    if(!fragmentSuccess)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, fragmentInfoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << fragmentInfoLog << std::endl;
    }
    // 16.创建着色器程序对象
    // glCreateProgram函数创建一个程序，并返回新创建程序对象的ID引用。
    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();
    // 17.编译的着色器附加到程序对象上，然后用glLinkProgram链接它们
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // 18.检测链接着色器程序是否失败，并获取相应的日志。
    int shaderProgramSuccess;
    char shaderProgramInfoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &shaderProgramSuccess);
    if(!shaderProgramSuccess) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, shaderProgramInfoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINK_FAILED\n" << shaderProgramInfoLog << std::endl;
    }
    // 在渲染循环中使用着色器程序。
    
    // 20.着色器对象链接到程序对象以后，记得删除着色器对象，我们不再需要它们了
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // 渲染设置
    // 启用顶点大小设置
    glEnable(GL_PROGRAM_POINT_SIZE);
    // 启用线框模式(Wireframe Mode)，在该模式下，默认是GL_FILL，这里是GL_LINE。
    // 在GL_LINE时，绘制模式为GL_TRIANGLES时，也是绘制的线框，除非使用GL_FILL，将填充。
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // 7.渲染循环(Render Loop)，它能在我们让GLFW退出前一直保持运行。
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
        // 绘制物体
        // 使用着色器程序
        glUseProgram(shaderProgram);
        // 绑定VAO（不需要每次都绑定，对于当前程序其实只需要绑定一次就可以了，因为我们没有更多的VAO了。）
        glBindVertexArray(VAO);
        // 绘制（绘制模式，起点，绘制几个点）
        //// 绘制点
        //// glDrawArrays(GL_POINTS, 0, 6);
        //// 绘制线段
        //// glDrawArrays(GL_LINE_LOOP, 0, 6);
        //// 绘制三角形
        //// glDrawArrays(GL_TRIANGLES, 0, 3);

        // 使用EBO索引缓冲对象来进行绘制
        glDrawElements(GL_LINE_LOOP, 6, GL_UNSIGNED_INT, 0);


        // 绘制完成后解绑VAO
        glBindVertexArray(0);

        // （3）检查并调用事件，交换缓冲
        glfwSwapBuffers(window);
        glfwPollEvents();    
    }

    // 8.渲染循环结束，进行资源释放
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    // 8.当渲染循环结束后我们需要正确释放/删除之前的分配的所有资源
    glfwTerminate();
    
    return 0;
}


