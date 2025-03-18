#ifndef BOX_GEOMETRY
#define BOX_GEOMETRY

#include <geometry/BufferGeometry.h>

using namespace std;

// 立方体几何体（继承BufferGeometry）
class BoxGeometry : public BufferGeometry
{
public:
    // 尺寸参数
    float width, height, depth;
    // 各方向分段数
    float widthSegments, heightSegments, depthSegments;

    // 构造函数
    BoxGeometry(float width = 1.0f, float height = 1.0f, float depth = 1.0, float widthSegments = 1.0f, float heightSegments = 1.0f, float depthSegments = 1.0f)
    {
        // 对分段数进行向下取整
        widthSegments = glm::floor(widthSegments);
        heightSegments = glm::floor(heightSegments);
        depthSegments = glm::floor(depthSegments);
        
        // 设置几何体的尺寸参数，包括宽度、高度、深度、各方向分段数
        this->width = width;
        this->height = height;
        this->depth = depth;
        this->widthSegments = widthSegments;
        this->heightSegments = heightSegments;
        this->depthSegments = depthSegments;
        
        // 生成平面
        /**
         * 三分量对应
         * vec3(u, v, w)
         * x y z | 0, 1, 2
         * z y x | 2, 1, 0
         * x z y | 0, 2, 1
         */
        this->buildPlane(2, 1, 0, -1, -1, depth, height, width, depthSegments, heightSegments, 0); // px
        this->buildPlane(2, 1, 0, 1, -1, depth, height, -width, depthSegments, heightSegments, 1); // nx
        
        this->buildPlane(0, 2, 1, 1, 1, width, depth, height, widthSegments, depthSegments, 2);   // py
        this->buildPlane(0, 2, 1, 1, -1, width, depth, -height, widthSegments, depthSegments, 3); // ny
        
        this->buildPlane(0, 1, 2, 1, -1, width, height, depth, widthSegments, heightSegments, 4);   // pz
        this->buildPlane(0, 1, 2, -1, -1, width, height, -depth, widthSegments, heightSegments, 5); // nz
        
        this->setupBuffers();
    }

private:
    // 顶点计数器
    int numberOfVertices = 0;
    // 组开始
    float groupStart = 0;

    // 辅助方法：生成平面
    /**
     * 三分量对应
     * vec3(u, v, w)
     * x y z | 0, 1, 2
     * z y x | 2, 1, 0
     * x z y | 0, 2, 1
     */
    void buildPlane(int u, int v, int w,       // z,y,x坐标轴映射 (0=x,1=y,2=z)  
                    float udir, float vdir,    // 方向控制 (+1/-1)
                    float width, float height, // 面尺寸
                    float depth,               // 面偏移量
                    float gridX, float gridY,  // 细分段数
                    float materialIndex        // 材质标识
                    )
    {
        // 段宽
        float segmentWidth = width / gridX;
        // 段高
        float segmentHeight = height / gridY;

        // 宽度一半
        float widthHalf = width / 2.0f;
        // 高度一半
        float heightHalf = height / 2.0f;
        // 深度一半
        float depthHalf = depth / 2.0f;

        // 宽度段数+1
        float gridX1 = gridX + 1.0f;
        // 高度段数+1
        float gridY1 = gridY + 1.0f;

        // 顶点计数器
        float vertexCounter = 0.0f;
        // 组计数器
        float groupCount = 0.0f;

        // 初始化向量
        glm::vec3 vector = glm::vec3(0.0f, 0.0f, 0.0f);

        // 创建顶点
        VertexProperty vertex;

        // 生成 顶点数据
        for (unsigned int iy = 0; iy < gridY1; iy++)
        {
            // y高度
            float y = iy * segmentHeight - heightHalf;
            for (unsigned int ix = 0; ix < gridX1; ix++)
            {
                // x宽度
                float x = ix * segmentWidth - widthHalf;

                // position
                vector[u] = x * udir;
                vector[v] = y * vdir;
                vector[w] = depthHalf;
                vertex.Position = glm::vec3(vector.x, vector.y, vector.z);

                // normals
                vector[u] = 0;
                vector[v] = 0;
                vector[w] = depth > 0 ? 1 : -1;
                vertex.Normal = glm::vec3(vector.x, vector.y, vector.z);

                // uvs
                vertex.TexCoords = glm::vec2(ix / gridX, 1 - (iy / gridY));

                this->vertices.push_back(vertex);

                // counters
                vertexCounter += 1.0;
            }
        }

        // indices
        // 生成 索引数据
        for (unsigned int iy = 0; iy < gridY; iy++)
        {
            for (unsigned int ix = 0; ix < gridX; ix++)
            {
                float a = numberOfVertices + ix + gridX1 * iy;
                float b = numberOfVertices + ix + gridX1 * (iy + 1);
                float c = numberOfVertices + (ix + 1) + gridX1 * (iy + 1);
                float d = numberOfVertices + (ix + 1) + gridX1 * iy;

                this->indices.push_back(a);
                this->indices.push_back(b);
                this->indices.push_back(d);

                this->indices.push_back(b);
                this->indices.push_back(c);
                this->indices.push_back(d);
            }
        }
        numberOfVertices += vertexCounter;
    }
};

#endif