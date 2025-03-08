#ifndef PLANE_GROMETRY
#define PLANE_GROMETRY

#include <geometry/BufferGeometry.h>

using namespace std;

// 平面几何体，继承自BufferGeometry
class PlaneGeometry : public BufferGeometry
{
public:
    // 构造函数（宽，高，宽段数（横向分割数），高段数（纵向分割数））
    PlaneGeometry(float width = 1.0, float height = 1.0, float wSegment = 1.0, float hSegment = 1.0)
    {
        // 计算宽高的一半
        float width_half = width / 2.0f;
        float height_half = height / 2.0f;

        // 计算网格的行列数
        /* 宽边的分段数为2，高边的分段数为2，就有：
        总顶点数 = gridX1 * gridY1 = (2+1)(2+1)=9个
        (wSegment=2, hSegment=2) 生成的顶点网格：
        (0,2)----(1,2)----(2,2)
          |        |        |
          |        |        |
        (0,1)----(1,1)----(2,1)
          |        |        |
          |        |        |
        (0,0)----(1,0)----(2,0)
        */
        float gridX1 = wSegment + 1.0f;
        float gridY1 = hSegment + 1.0f;

        // 计算每个网格的宽高，宽边单位长度，高边单位长度。
        float segment_width = width / wSegment;
        float segment_height = height / hSegment;

        // 创建顶点，这里顶点结构体是继承于BufferGeometry中的结构体
        Vertex vertex;

        // 生成顶点位置、法向量、纹理坐标
        // 遍历每个网格
        for (int iy = 0; iy < gridY1; iy++)
        {
            // 计算每个网格的y坐标
            float y = iy * segment_height - height_half;

            // 遍历每个网格
            for (int ix = 0; ix < gridX1; ix++)
            {
                // 计算每个网格的x坐标
                float x = ix * segment_width - width_half;

                // 设置顶点位置、法向量、纹理坐标
                // 顶点位置（x,y,z，z设置为0，表示平面在xy平面上，原点并不是左下角，而是中心点）
                vertex.Position = glm::vec3(x, -y, 0.0f);
                // 法向量（法向量是垂直于面的单位向量）
                vertex.Normal = glm::vec3(0.0f, 0.0f, 1.0f);
                // 纹理坐标（范围在0-1之间，纹理坐标的原点在左下角）
                vertex.TexCoords = glm::vec2(ix / wSegment, 1.0f - (iy / hSegment));

                // 添加顶点
                this->vertices.push_back(vertex);
            }
        }

        // 生成索引
        for (int iy = 0; iy < hSegment; iy++) // 遍历高度方向的分段
        {
            for (int ix = 0; ix < wSegment; ix++) // 遍历宽度方向的分段
            {
                // 计算当前网格的四个顶点索引
                // a---d
                // | / |
                // b---c

                // 计算每个网格的索引
                float a = ix + gridX1 * iy; // 左下顶点
                float b = ix + gridX1 * (iy + 1); // 左上顶点
                float c = (ix + 1) + gridX1 * (iy + 1); // 右上顶点
                float d = (ix + 1) + gridX1 * iy; // 右下顶点

                // 添加索引
                // 每一个网格单元由两个三角形组成（逆时针顺序）
                // 组成两个三角形（逆时针顺序）
                // 第一个三角形：a → b → d
                this->indices.push_back(a);
                this->indices.push_back(b);
                this->indices.push_back(d);
                // 第二个三角形：d → b → c
                this->indices.push_back(d);
                this->indices.push_back(b);
                this->indices.push_back(c);
            }
        }

        // 初始化缓冲区
        this->setupBuffers();
    }
};

#endif