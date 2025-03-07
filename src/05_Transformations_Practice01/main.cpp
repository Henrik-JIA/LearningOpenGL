#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <tool/Shader.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <tool/stb_image.h>

// 着色器代码
const char *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;
out vec3 ourColor;
out vec3 ourPos;
out vec2 TexCoord;

uniform float factor;
uniform mat4 transform;

mat4 rotate3d(float _angle)
{
    return mat4(
        cos(_angle), -sin(_angle), 0.0f, 0.0f,
        sin(_angle), cos(_angle), 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

void main()
{
    // gl_Position = vec4(rotate3d(factor) * vec4(aPos, 1.0f));
    gl_Position = vec4(transform * vec4(aPos, 1.0f));
    gl_PointSize = 10.0f;

    ourColor = aColor;
    ourPos = aPos;
    TexCoord = aTexCoord * 2.0f;
}
)";

const char *fragmentShaderSource =  R"(
#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec3 ourPos;
in vec2 TexCoord;

uniform float factor;

uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
    // FragColor = vec4(ourColor, 1.0);
    // FragColor = texture(ourTexture, TexCoord) * vec4(ourColor, 1.0);
    FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), abs(sin(factor)));

    // float xy = length(ourPos.xy);
    // FragColor = mix(texture(texture1, TexCoord), vec4(ourColor, 1.0 - xy * 2.0) * texture(texture2, TexCoord), 0.1);
}
)";

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
    // =====================窗件窗口与视口===================================
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

    // 4.创建视口，使用OpenGL函数，可以将视口的维度设置为比GLFW的维度小，
    // 这样子之后所有的OpenGL渲染将会在一个更小的窗口中显示，这样子的话我们也可以将一些其它元素显示在OpenGL视口之外。
    glViewport(0, 0, 800, 600); // 这里是视口

    // 5.创建回调函数framebuffer_size_callback，以实现视口随着窗口调整而调整。
    // 需要注册这个framebuffer_size_callback函数（监听），告诉GLFW我们希望每当窗口调整大小的时候调用这个函数
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // ==========================================================================
    
    // ====================顶点做色器程序与片段做色器程序=========================
    // 顶点数据
    float vertices[] = {
        //     ---- 位置 ----       ---- 颜色 ----     - 纹理坐标 -
         0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // 右上
         0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // 右下
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // 左下
        -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // 左上
    };
    unsigned int indices[] = {
        0, 1, 3, // 第一个三角形
        1, 2, 3  // 第二个三角形
    };
    
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO); 
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // 设置顶点位置属性指针
    // 参数1：属性位置（对应shader中的layout(location=1)）
    // 参数2：属性分量数量（颜色使用RGB 3个分量）
    // 参数3：数据类型（浮点型）
    // 参数4：是否归一化（true会把数据映射到0-1或-1-1）
    // 参数5：步长（每个顶点数据的总字节长度）
    // 参数6：起始偏移量（颜色数据在顶点结构中的起始位置）
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 设置顶点颜色属性指针
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3* sizeof(float)));
    glEnableVertexAttribArray(1);

    // 设置顶点纹理坐标属性指针
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    // 从文件加载（原注释掉的版本）
    // Shader ourShader = Shader::FromFile("../src/03_Shaders03/shader/vertex.glsl", 
    //                                     "../src/03_Shaders03/shader/fragment.glsl");
    // 从源码加载（原直接使用字符串的版本）
    Shader ourShader = Shader::FromSource(vertexShaderSource, fragmentShaderSource);

    // 获取mixValue的位置
    GLint locFactor = ourShader.getUniformLocation("factor");
    
    // ===================================================================

    // ==================================设置纹理对象================================
    unsigned int texture1, texture2;
    // 第一个纹理
    glGenTextures(1, &texture1); // 生成一个
    glBindTexture(GL_TEXTURE_2D, texture1); 
    
    // 为当前绑定的纹理对象设置环绕方式、过滤方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);   
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // 缩小时的过滤方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // 邻近过滤
    // 放大时的过滤方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // 线性过滤，使用线性插值

    // 设置图像翻转，因为OpenGL要求y轴0.0坐标是在图片的底部的，但是图片的y轴0.0坐标通常在顶部。
    stbi_set_flip_vertically_on_load(true);

    // 加载纹理图片
    int width, height, nrChannels;
    unsigned char *data = stbi_load("../static/texture/container.jpg", &width, &height, &nrChannels, 0);
    // 载入的图片数据生成一个纹理
    if(data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        // 使用多级渐远纹理，我们必须手动设置所有不同的图像（不断递增第二个参数）。
        // 或者，直接在生成纹理之后调用glGenerateMipmap。
        // 这会为当前绑定的纹理自动生成所有需要的多级渐远纹理。
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    // 生成了纹理和相应的多级渐远纹理后，释放图像的内存
    stbi_image_free(data);

    // 第二个纹理
    glGenTextures(1, &texture2); // 生成一个
    // 在绑定纹理之前先激活纹理单元，默认激活GL_TEXTURE0，这里可以不用激活，如果是需要存入其他的纹理单元中就需要激活，一共有16个纹理单元。
    // 可以通过GL_TEXTURE0 + 8的方式获得GL_TEXTURE8，这在当我们需要循环一些纹理单元的时候会很有用。
    // 此外如果是需要存入其他的纹理单元中，还需要给纹理采样器分配一个位置值，需要通过做色器类中的uniform相关的set进行赋值。
    glActiveTexture(GL_TEXTURE1); // 重要：这里激活GL_TEXTURE1纹理单元，后续操作默认影响最后激活的单元
    // 绑定当前纹理，让之后任何的纹理指令都可以配置当前绑定的纹理
    glBindTexture(GL_TEXTURE_2D, texture2); 

    // 为当前绑定的纹理对象设置环绕方式、过滤方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);   
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // 缩小时的过滤方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // 邻近过滤
    // 放大时的过滤方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // 线性过滤，使用线性插值

    // 设置图像翻转，因为OpenGL要求y轴0.0坐标是在图片的底部的，但是图片的y轴0.0坐标通常在顶部。
    stbi_set_flip_vertically_on_load(true);

    // 加载纹理图片，这里可以不用新建宽、高、通道数、data，直接用上面创建好的变量。
    data = stbi_load("../static/texture/awesomeface.png", &width, &height, &nrChannels, 0);
    // 载入的图片数据生成一个纹理
    if(data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        // 使用多级渐远纹理，我们必须手动设置所有不同的图像（不断递增第二个参数）。
        // 或者，直接在生成纹理之后调用glGenerateMipmap。
        // 这会为当前绑定的纹理自动生成所有需要的多级渐远纹理。
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    // 生成了纹理和相应的多级渐远纹理后，释放图像的内存
    stbi_image_free(data);
    
    // 给纹理采样器分配一个位置值，需要通过做色器类中的uniform相关的set进行赋值。
    // ourShader.setInt("texture1", 0);
    // ourShader.setInt("texture2", 1);
    // 先获取uniform着色器中对应的位置。
    GLint locTexture1 = ourShader.getUniformLocation("texture1");
    GLint locTexture2 = ourShader.getUniformLocation("texture2");
    // // 给纹理采样器分配一个位置值，需要通过做色器类中的uniform相关的set进行赋值。
    // ourShader.setInt(locTexture, 1);

    // ========================================================================

    // ==========================矩阵变换======================================

    // 初始化单位矩阵
    glm::mat4 trans = glm::mat4(1.0f);
    // 绕z轴旋转45度
    trans = glm::rotate(trans, glm::radians(45.0f), glm::vec3(0.0, 0.0, 1.0));
    trans = glm::scale(trans, glm::vec3(0.5, 0.5, 0.5));
    // 先获取uniform着色器中对应的位置。
    GLint locTransform = ourShader.getUniformLocation("transform");
    

    // =======================================================================

    // 渲染设置
    // 启用顶点大小设置
    glEnable(GL_PROGRAM_POINT_SIZE);
    // 启用线框模式(Wireframe Mode)，在该模式下，默认是GL_FILL，这里是GL_LINE。
    // 在GL_LINE时，绘制模式为GL_TRIANGLES时，也是绘制的线框，除非使用GL_FILL，将填充。
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    // 初始化mixValue
    float factor = 0.0f;

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
        // glUseProgram(shaderProgram);
        ourShader.use();

        // 设置mixValue
        factor = glfwGetTime();
        ourShader.setFloat(locFactor, factor);

        // 绘制第一个物体
        // 设置transform
        // 先旋转后平移：物体会先绕原点旋转，然后平移到目标位置。
        trans = glm::translate(trans, glm::vec3(-0.5, 0.0, 0.0));
        trans = glm::rotate(trans, glm::radians(factor * 20.0f), glm::vec3(0.0, 0.0, 1.0));
        trans = glm::scale(trans, glm::vec3(0.5, 0.5, 0.5));
        // 先平移后旋转：物体会先移动到新位置，然后绕原点旋转（会产生"公转"效果）。
        // trans = glm::scale(trans, glm::vec3(0.5, 0.5, 0.5));
        // trans = glm::rotate(trans, glm::radians(factor * 10.0f), glm::vec3(0.0, 0.0, 1.0));
        // trans = glm::translate(trans, glm::vec3(0.5, 0.0, 0.0));
        ourShader.setMat4(locTransform, trans);
        // 重置trans
        trans = glm::mat4(1.0f);

        // 给纹理采样器分配一个位置值，需要通过做色器类中的uniform相关的set进行赋值。
        ourShader.setInt(locTexture1, 0);
        ourShader.setInt(locTexture2, 1);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture2);


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
        glDrawElements(GL_POINTS, 6, GL_UNSIGNED_INT, 0);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        // 绘制第二个物体（通过调整位置、缩放、旋转来控制点的位置，并重新绘制）
        // 重新设置transform
        trans = glm::translate(trans, glm::vec3(0.5, 0.0, 0.0));
        // 注意：这里如果z轴也使用sin函数，会存在z轴的值反转，图像会颠倒。
        trans = glm::scale(trans, glm::vec3(sin(factor) * 0.5, sin(factor) * 0.5, sin(factor) *  0.5));
        ourShader.setMat4(locTransform, trans);
        // 重置trans
        trans = glm::mat4(1.0f);
        // 绘制新的位置
        glDrawElements(GL_POINTS, 6, GL_UNSIGNED_INT, 0);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


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

    // 8.当渲染循环结束后我们需要正确释放/删除之前的分配的所有资源
    glfwTerminate();
    
    return 0;
}


