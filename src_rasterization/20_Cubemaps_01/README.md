## drawSkyBox 函数差异说明

### main.cpp 与 main copy.cpp 的差异

1. **参数列表不同**：
   - main.cpp 版本：
     ```cpp
     void drawSkyBox(Shader& shader, BoxGeometry& geometry, unsigned int& cubeMap)
     ```
     在函数内部自行计算 view 和 projection 矩阵：
     ```cpp
     glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
     glm::mat4 projection = glm::perspective(...);
     ```

   - main copy.cpp 版本：
     ```cpp
     void drawSkyBox(Shader& shader, BoxGeometry& geometry, unsigned int& cubeMap, 
                    glm::mat4& view, glm::mat4& projection)
     ```
     需要外部传入 view 和 projection 矩阵

2. **参数传递方式**：
   - 所有参数**必须使用引用传递**（`&`符号）
   - 原因：
     - 避免大型对象（Shader/BoxGeometry/矩阵）的拷贝开销
     - 允许函数直接操作原始资源（如纹理ID）
     - 保持OpenGL对象状态的一致性

3. **参数说明**：
   | 参数        | 类型             | 说明                          |
   |------------|------------------|-----------------------------|
   | shader     | Shader&          | 天空盒着色器程序（必须引用）         |
   | geometry   | BoxGeometry&     | 天空盒几何体数据（必须引用）         |
   | cubeMap    | unsigned int&    | 立方体贴图ID（必须引用）           |
   | view       | glm::mat4&       | 仅copy版本需要，观察矩阵（必须引用）  |
   | projection | glm::mat4&       | 仅copy版本需要，投影矩阵（必须引用）  |