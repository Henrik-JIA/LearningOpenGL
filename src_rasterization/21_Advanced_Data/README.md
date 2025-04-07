# 高级数据

## 缓冲

到目前为止，我们一直是调用glBufferData函数来填充缓冲对象所管理的内存，这个函数会分配一块GPU内存，并将数据添加到这块内存中。如果我们将它的`data`参数设置为`NULL`，那么这个函数将只会分配内存，但不进行填充。这在我们需要**预留**(Reserve)特定大小的内存，之后回到这个缓冲一点一点填充的时候会很有用。

### 方法1：传统数据更新操作流程解析

除了使用一次函数调用填充整个缓冲之外，我们也可以使用glBufferSubData，填充缓冲的特定区域。这个函数需要一个缓冲目标、一个偏移量、数据的大小和数据本身作为它的参数。这个函数不同的地方在于，我们可以提供一个偏移量，指定从**何处**开始填充这个缓冲。这能够让我们插入或者更新缓冲内存的某一部分。要注意的是，缓冲需要有足够的已分配内存，所以对一个缓冲调用glBufferSubData之前必须要先调用glBufferData。

1. 准备数据阶段

   - 在 CPU 内存 中准备好需要上传的数据（如 data 数组）

   - 数据暂存于应用程序的内存空间，与 GPU 显存完全隔离

2. API调用阶段

   - 通过 glBufferSubData 发起数据传输请求

   - OpenGL 驱动会 创建临时副本：A[应用内存 data] --> B[驱动层中转内存] --> C[GPU显存目标区域]

   - 此时应用程序内存与 GPU 显存 没有直接连接

3. 数据传输阶段

   - 驱动层执行**双重拷贝**：

     1. 从应用内存拷贝到驱动管理的中转内存

     2. 从中转内存拷贝到最终显存位置

   - 数据提交 立即生效（除非使用异步扩展）

   - 原显存数据被 直接覆盖，无需旧数据释放操作

```c++
glBufferSubData(GL_ARRAY_BUFFER, 24, sizeof(data), &data); // 范围： [24, 24 + sizeof(data)]
```

### 方法2：内存映射操作流程解析

将数据导入缓冲的另外一种方法是，请求缓冲内存的指针，直接将数据复制到缓冲当中。通过调用glMapBuffer函数，OpenGL会返回当前绑定缓冲的内存指针，供我们操作：

1. glMapBuffer 阶段

   - 获取 GPU 缓冲区在 CPU 可访问内存空间的映射指针

   - OpenGL 会临时将显存数据映射到 CPU 可访问的内存区域

   - 此时你可以通过指针 ptr 直接修改显存数据

2. memcpy 阶段

   - 将 data 数组内容复制到映射内存区域

   - 这相当于把 CPU 内存中的数据 直接写入显存映射区

   - 此时数据尚未真正提交到 GPU

3. glUnmapBuffer 阶段

   - 解除内存映射：断开 CPU 指针 ptr 与显存的关联

   - 提交修改：将映射期间的所有改动 同步到 GPU 显存

   - 不清理数据：原缓冲区数据会被新数据覆盖，旧数据自动释放
   - 当我们使用glUnmapBuffer函数，告诉OpenGL我们已经完成指针操作之后，OpenGL就会知道你已经完成了。在解除映射(Unmapping)之后，指针将会不再可用，并且如果OpenGL能够成功将您的数据映射到缓冲中，这个函数将会返回GL_TRUE。

```c++
float data[] = {
  0.5f, 1.0f, -0.35f
  ...
};
glBindBuffer(GL_ARRAY_BUFFER, buffer);
// 获取指针
void *ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
// 复制数据到内存
memcpy(ptr, data, sizeof(data));
// 记得告诉OpenGL我们不再需要这个指针了
glUnmapBuffer(GL_ARRAY_BUFFER);
```

如果要直接映射数据到缓冲，而不事先将其存储到临时内存中，glMapBuffer这个函数会很有用。比如说，你可以从文件中读取数据，并直接将它们复制到缓冲内存中。

### 传统数据更新 vs 内存映射

| 特性     | glBufferSubData                        | 内存映射 (glMapBuffer + glUnmapBuffer) |
| -------- | -------------------------------------- | -------------------------------------- |
| 数据流   | CPU内存 → 驱动层 → GPU显存（双重拷贝） | CPU直接写入显存映射区（单次拷贝）      |
| API调用  | 显式函数调用                           | 需要获取/释放内存指针                  |
| 性能     | 适合中小数据量                         | 适合大数据量/频繁更新                  |
| 同步控制 | 立即执行（可能阻塞渲染线程）           | 可异步操作（需手动同步）               |
| 内存管理 | OpenGL驱动自动管理                     | 开发者直接操作映射内存                 |



## 分批顶点属性

### 方法1：交错方式

通过使用glVertexAttribPointer，我们能够指定顶点数组缓冲内容的属性布局。在顶点数组缓冲中，我们对属性进行了交错(Interleave)处理，也就是说，我们将每一个顶点的位置、法线和/或纹理坐标紧密放置在一起。交错布局123123123123。

```c++
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
    glGenVertexArrays(1, &VAO); // 创建顶点数组对象
    glGenBuffers(1, &VBO); // 创建顶点缓冲对象
    glGenBuffers(1, &EBO); // 创建元素缓冲对象

    glBindVertexArray(VAO); // 开始记录VAO状态
	// 上传顶点数据到VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO); 
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	// 上传索引数据到EBO
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
    glEnableVertexAttribArray(0); // 启用该属性通道

    // 设置顶点颜色属性指针
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3* sizeof(float)));
    glEnableVertexAttribArray(1);

    // 设置顶点纹理坐标属性指针
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0); // 结束VAO配置
```



### 方法2：分批方式

我们可以做的是，将每一种属性类型的向量数据打包(Batch)为一个大的区块，而不是对它们进行交错储存。与交错布局123123123123不同，我们将采用分批(Batched)的方式111122223333。

当从文件中加载顶点数据的时候，你通常获取到的是一个位置数组、一个法线数组和/或一个纹理坐标数组。我们需要花点力气才能将这些数组转化为一个大的交错数据数组。使用分批的方式会是更简单的解决方案，我们可以很容易使用glBufferSubData函数实现：
```c++
// 假设各数组长度相同（顶点数相同）
float positions[] = { /* 顶点1位置, 顶点2位置... */ }; // 每个元素3个float
float normals[] = { /* 顶点1法线, 顶点2法线... */ };   // 每个元素3个float
float tex[] = { /* 顶点1纹理坐标, 顶点2纹理坐标... */ }; // 每个元素2个float

// 计算总缓冲大小
GLsizeiptr totalSize = sizeof(positions) + sizeof(normals) + sizeof(tex);

// 创建缓冲
glBufferData(GL_ARRAY_BUFFER, totalSize, NULL, GL_STATIC_DRAW);

// 填充缓冲
glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(positions), &positions); // 位置块
glBufferSubData(GL_ARRAY_BUFFER, sizeof(positions), sizeof(normals), &normals); // 法线块
glBufferSubData(GL_ARRAY_BUFFER, sizeof(positions) + sizeof(normals), sizeof(tex), &tex); // 纹理块
```

这样子我们就能直接将属性数组作为一个整体传递给缓冲，而不需要事先处理它们了。我们仍可以将它们合并为一个大的数组，再使用glBufferData来填充缓冲，但对于这种工作，使用glBufferSubData会更合适一点。

我们还需要更新顶点属性指针来反映这些改变：
内存布局示意图：

```txt
+----------------+----------------+----------------+
| 位置数据块      | 法线数据块      | 纹理数据块      |
| (N×12字节)     | (N×12字节)      | (N×8字节)      |
+----------------+----------------+----------------+
0                ↑                ↑                
                sizeof(pos)      sizeof(pos)+sizeof(normal)
```

```c++
// 位置属性 (location=0)
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 
                      3 * sizeof(float), // stride = 属性块内单个顶点的步长
                      0); // 起始偏移量

// 法线属性 (location=1)
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                      3 * sizeof(float), 
                      (void*)(sizeof(positions))); // 法线块起始位置

// 纹理坐标属性 (location=2)
glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                      2 * sizeof(float),
                      (void*)(sizeof(positions) + sizeof(normals))); // 纹理块起始位置
```

注意`stride`参数等于顶点属性的大小，因为下一个顶点属性向量能在3个（或2个）分量之后找到。

这给了我们设置顶点属性的另一种方法。使用哪种方法都是可行的，它只是设置顶点属性的一种更整洁的方式。但是，**推荐使用交错方法，因为这样一来，每个顶点着色器运行时所需要的顶点属性在内存中会更加紧密对齐**。

###  两种顶点数据布局对比

| 布局类型 | 交错布局 (Interleaved)               | 分批布局 (Batched)                                 |
| -------- | ------------------------------------ | -------------------------------------------------- |
| 内存结构 | 顶点属性交替存储：[位置,法线,纹理]×N | 属性分块存储：[所有位置] / [所有法线] / [所有纹理] |
| 优点     | 缓存友好，适合现代GPU                | 数据准备简单，无需重组数组                         |
| 缺点     | 需要预处理合并数据                   | 可能降低缓存命中率                                 |
| 适用场景 | 高性能渲染                           | 快速原型开发/简单模型                              |



## 缓冲复制机制解析

### 方法1：在GPU显存中直接复制缓冲数据

当你的缓冲已经填充好数据之后，你可能会想与其它的缓冲共享其中的数据，或者想要将缓冲的内容复制到另一个缓冲当中。glCopyBufferSubData能够让我们相对容易地从一个缓冲中复制数据到另一个缓冲中。这个函数的原型如下：
```c++
void glCopyBufferSubData(GLenum readtarget, GLenum writetarget, GLintptr readoffset,
                         GLintptr writeoffset, GLsizeiptr size);
```

`readtarget`和`writetarget`参数需要填入复制源和复制目标的缓冲目标。比如说，我们可以将VERTEX_ARRAY_BUFFER缓冲复制到VERTEX_ELEMENT_ARRAY_BUFFER缓冲，分别将这些缓冲目标设置为读和写的目标。当前绑定到这些缓冲目标的缓冲将会被影响到。

但如果我们想读写数据的两个不同缓冲都为顶点数组缓冲该怎么办呢？我们不能同时将两个缓冲绑定到同一个缓冲目标上。正是出于这个原因，OpenGL提供给我们另外两个缓冲目标，叫做GL_COPY_READ_BUFFER和GL_COPY_WRITE_BUFFER，GL_COPY_* 目标专为缓冲间复制设计，避免与常规操作目标冲突，不会影响当前绑定的GL_ARRAY_BUFFER等常规目标，数据直接在GPU显存间传输，无需经过CPU内存（性能优势）。我们接下来就可以将需要的缓冲绑定到这两个缓冲目标上，并将这两个目标作为`readtarget`和`writetarget`参数。

接下来glCopyBufferSubData会从`readtarget`中读取`size`大小的数据，并将其写入`writetarget`缓冲的`writeoffset`偏移量处。下面这个例子展示了如何复制两个顶点数组缓冲：
```c++
glBindBuffer(GL_COPY_READ_BUFFER, vbo1);
glBindBuffer(GL_COPY_WRITE_BUFFER, vbo2);
glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sizeof(vertexData));
```

#### 典型应用场景

1. 双缓冲动画系统
   ```c++
      // 帧1：使用vbo1渲染
      // 帧2：将vbo1数据复制到vbo2并修改
      glCopyBufferSubData(..., sizeof(animData));
   ```

2. 物理模拟数据交换

   ```c++
      // 将计算结果从物理缓冲复制到渲染缓冲
      glCopyBufferSubData(GL_COPY_READ_BUFFER, physicsVBO,
                          GL_COPY_WRITE_BUFFER, renderVBO,
                          0, 0, particleCount * sizeof(Particle));
   ```

3. LOD数据切换

   ```c++
      // 根据视距切换不同细节层次的模型数据
      if (distance < 100) {
          glCopyBufferSubData(..., highLODVBO, ..., lowLODVBO, ...);
      }
   ```

   

### 方法2：缓冲目标混合使用机制

OpenGL允许在复制操作中 混合使用常规缓冲目标和专用复制目标，我们也可以只将`writetarget`缓冲绑定为新的缓冲目标类型之一，其核心规则是：
```c++
float vertexData[] = { ... };
glBindBuffer(GL_ARRAY_BUFFER, vbo1); // 源绑定到常规目标
glBindBuffer(GL_COPY_WRITE_BUFFER, vbo2); // 目标绑定到复制目标
glCopyBufferSubData(GL_ARRAY_BUFFER, // 源目标类型
                    GL_COPY_WRITE_BUFFER, // 目标目标类型
                    0, 0, sizeof(vertexData) 
                   );
```

源和目标可以使用任意不同的缓冲目标类型，实际操作的缓冲对象由当前绑定到目标类型的缓冲决定，允许不中断当前渲染管线状态进行数据复制。

#### 应用场景示例

场景：在渲染循环中动态更新顶点数据
```c++
// 初始化阶段
glBindBuffer(GL_ARRAY_BUFFER, vbo_dynamic); // 常规目标绑定动态VBO

// 渲染循环中
while (rendering) {
    // 从静态VBO复制数据到动态VBO（不打断当前VAO状态）
    glBindBuffer(GL_COPY_READ_BUFFER, vbo_static);  // 专用读目标
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_ARRAY_BUFFER, 
                       0, 0, sizeof(staticData));
    
    // 继续使用GL_ARRAY_BUFFER进行渲染
    glDrawArrays(...);
}
```

### 特别注意

在OpenGL中，不能将读目标和写目标同时设置为同一个常规缓冲目标类型（如GL_ARRAY_BUFFER）。这是由OpenGL的缓冲绑定机制决定的，具体原因如下：

#### 错误示例分析：

```c++
// 错误用法：尝试将同一目标类型用于读写
glBindBuffer(GL_ARRAY_BUFFER, vbo1);  // 绑定源缓冲
glBindBuffer(GL_ARRAY_BUFFER, vbo2);  // 覆盖绑定，现在GL_ARRAY_BUFFER绑定的是vbo2
glCopyBufferSubData(GL_ARRAY_BUFFER,  // 源目标类型
                    GL_ARRAY_BUFFER,  // 目标目标类型
                    0, 0, size);      // 实际源和目标都是vbo2！
```

- 问题1：第二次glBindBuffer会解除vbo1的绑定

- 问题2：源和目标实际指向同一个缓冲vbo2

- 结果：产生GL_INVALID_OPERATION错误

#### 正确做法：

必须使用 不同的缓冲目标类型 来区分源和目标：

```c++
// 方法1：混合使用常规目标和复制目标
glBindBuffer(GL_ARRAY_BUFFER, vbo1);        // 源绑定到常规目标
glBindBuffer(GL_COPY_WRITE_BUFFER, vbo2);   // 目标绑定到复制目标
glCopyBufferSubData(GL_ARRAY_BUFFER, GL_COPY_WRITE_BUFFER, ...);

// 方法2：使用专用复制目标
glBindBuffer(GL_COPY_READ_BUFFER, vbo1);
glBindBuffer(GL_COPY_WRITE_BUFFER, vbo2);
glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, ...);
```

#### 实际应用建议：

1. 优先使用专用复制目标：完全隔离复制操作与渲染管线状态

```c++
   glBindBuffer(GL_COPY_READ_BUFFER, srcVBO);
   glBindBuffer(GL_COPY_WRITE_BUFFER, dstVBO);
   glCopyBufferSubData(...);
```

2. 临时混合使用时注意

```c++
   // 保存当前绑定状态
   GLint prevArrayBuffer;
   glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevArrayBuffer);

   // 执行复制
   glBindBuffer(GL_ARRAY_BUFFER, srcVBO);
   glBindBuffer(GL_COPY_WRITE_BUFFER, dstVBO);
   glCopyBufferSubData(...);

   // 恢复原状态
   glBindBuffer(GL_ARRAY_BUFFER, prevArrayBuffer);
```

