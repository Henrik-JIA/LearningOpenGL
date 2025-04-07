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

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

int main(){
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);  // 设置OpenGL主版本号为3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);  // 设置OpenGL次版本号为3
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 使用核心模式

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 向前兼容模式（启用后会移除所有被标记为已废弃的OpenGL功能）
#endif 

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout<< "Failed to create GLFW window!" << std::endl;
        // 窗口创建失败，则释放所有glfw资源
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // GLAD的初始化必须放在创建OpenGL上下文（glfwMakeContextCurrent）之后，但在调用任何OpenGL函数之前。
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // 创建视口，使用OpenGL函数
    glViewport(0, 0, 800, 600);

    // 创建回调函数framebuffer_size_callback，以实现视口随着窗口调整而调整。
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    float vertices[] = {
        // 第一个三角形
        -0.5f, 0.5f, 0.0f,
        -0.75f, -0.5f, 0.0f,
        -0.25f, -0.5f, 0.0f,
        // 第二个三角形
        0.5f, 0.5f, 0.0f,
        0.75f, -0.5f, 0.0f,
        0.25f, -0.5f, 0.0f
    };
    
    unsigned int indices[] = {
        0, 1, 2, // 第一个三角形
        3, 4, 5  // 第二个三角形
    };

    // =============典型渲染流程================
    // 生成顶点数组对象(VAO)、顶点缓冲对象(VBO)和索引缓冲对象(EBO)的ID
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);  // 创建1个VAO，ID存入VAO变量
    glGenBuffers(1, &VBO);       // 创建1个VBO，ID存入VBO变量
    glGenBuffers(1, &EBO);       // 创建1个EBO，ID存入EBO变量
    // 开始记录顶点属性配置（VAO绑定后才会记录后续操作）
    glBindVertexArray(VAO);      // 绑定VAO到当前上下文
    // 配置顶点缓冲
    glBindBuffer(GL_ARRAY_BUFFER, VBO);  // 绑定VBO到顶点缓冲目标
    // 将CPU的vertices数组数据复制到GPU的VBO中
    // GL_STATIC_DRAW表示数据几乎不变
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // 配置索引缓冲（必须在VAO绑定期间操作）
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);  // 绑定EBO到索引缓冲目标
    // 将CPU的indices数组数据复制到GPU的EBO中
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    // 设置顶点属性解析规则
    // 参数说明：
    // 0：对应shader中的layout(location=0)
    // 3：每个顶点有3个分量（x,y,z）
    // GL_FLOAT：数据类型为浮点数
    // GL_FALSE：不需要标准化
    // 3*sizeof(float)：步长（连续顶点之间的字节间隔）
    // (void*)0：起始偏移量
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);  // 启用顶点属性位置0
    // 解绑VAO（结束状态记录）
    glBindVertexArray(0);  // 0表示解绑当前VAO
    // ======================================== 

    // ================= 着色器 ======================
    // 顶点着色器与、段着色器、着色器程序对象
    unsigned int vertexShader, fragmentShader, shaderProgram;
    // 创建着色器对象和着色器程序对象
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    shaderProgram = glCreateProgram();
    // 把这个着色器源码附加到着色器对象上，然后编译它。
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // 用于检测是否编译成功
    int vertexSuccess, fragmentSuccess, shaderProgramSuccess;
    char vertexInfoLog[512], fragmentInfoLog[512], shaderProgramInfoLog[512];
    // 测是否编译成功
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vertexSuccess);
    if(!vertexSuccess)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, vertexInfoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << vertexInfoLog << std::endl;
    }
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fragmentSuccess);
    if(!fragmentSuccess)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, fragmentInfoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << fragmentInfoLog << std::endl;
    }

    // 编译成功后，检测着色器编译成功后，编译的着色器附加到程序对象上，然后用glLinkProgram链接它们
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // 检测链接着色器程序是否失败，并获取相应的日志。
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &shaderProgramSuccess);
    if(!shaderProgramSuccess) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, shaderProgramInfoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINK_FAILED\n" << shaderProgramInfoLog << std::endl;
    }
    // 在渲染循环中使用着色器程序。

    // 着色器对象链接到程序对象以后，记得删除着色器对象，我们不再需要它们了
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    // =================================================


    // 渲染设置
    // 启用顶点大小设置
    glEnable(GL_PROGRAM_POINT_SIZE);
    // 启用线框模式(Wireframe Mode)，在该模式下，默认是GL_FILL，这里是GL_LINE。
    // 在GL_LINE时，绘制模式为GL_TRIANGLES时，也是绘制的线框，除非使用GL_FILL，将填充。
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // 渲染循环(Render Loop)，它能在我们让GLFW退出前一直保持运行。
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
        glDrawElements(
            GL_TRIANGLES,     // 绘制模式：将顶点按顺序连接成线框，并闭合
            6,                // 索引数量：使用6个索引（两个三角形）
            GL_UNSIGNED_INT,  // 索引数据类型：无符号整数
            0                 // 索引数据偏移量：从索引缓冲的起始位置开始
        );


        // 绘制完成后解绑VAO
        glBindVertexArray(0);

        // （3）检查并调用事件，交换缓冲
        glfwSwapBuffers(window);
        glfwPollEvents();    
    }

    // 渲染循环结束，进行资源释放
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    // 当渲染循环结束后我们需要正确释放/删除之前的分配的所有资源
    glfwTerminate();
    
    return 0;
}


