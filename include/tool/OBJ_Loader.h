//
// Created by LEI XU on 4/28/19.
//
//
// This loader is created by Robert Smith.
// https://github.com/Bly7/OBJ-Loader
// Use the MIT license.


#ifndef RASTERIZER_OBJ_LOADER_H
#define RASTERIZER_OBJ_LOADER_H

#pragma once


#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <math.h>

// Print progress to console while loading (large models)
#define OBJL_CONSOLE_OUTPUT

// Namespace: OBJL
//
// Description: The namespace that holds eveyrthing that
//	is needed and used for the OBJ Model Loader
namespace objl
{
    // Structure: Vector2
    //
    // Description: 2D 向量，包含位置数据
    struct Vector2
    {
        // 默认构造函数
        Vector2()
        {
            X = 0.0f;
            Y = 0.0f;
        }
        // 变量设置构造函数
        Vector2(float X_, float Y_)
        {
            X = X_;
            Y = Y_;
        }
        // 相等运算符重载
        bool operator==(const Vector2& other) const
        {
            return (this->X == other.X && this->Y == other.Y);
        }
        // 不相等运算符重载
        bool operator!=(const Vector2& other) const
        {
            return !(this->X == other.X && this->Y == other.Y);
        }
        // 加法运算符重载
        Vector2 operator+(const Vector2& right) const
        {
            return Vector2(this->X + right.X, this->Y + right.Y);
        }
        // 减法运算符重载
        Vector2 operator-(const Vector2& right) const
        {
            return Vector2(this->X - right.X, this->Y - right.Y);
        }
        // 浮点乘法运算符重载
        Vector2 operator*(const float& other) const
        {
            return Vector2(this->X *other, this->Y * other);
        }

        // 位置变量
        float X;
        float Y;
    };

    // Structure: Vector3
    //
    // Description: 3D 向量，包含位置数据
    struct Vector3
    {
        // 默认构造函数
        Vector3()
        {
            X = 0.0f;
            Y = 0.0f;
            Z = 0.0f;
        }
        // 变量设置构造函数
        Vector3(float X_, float Y_, float Z_)
        {
            X = X_;
            Y = Y_;
            Z = Z_;
        }
        // 相等运算符重载
        bool operator==(const Vector3& other) const
        {
            return (this->X == other.X && this->Y == other.Y && this->Z == other.Z);
        }
        // 不相等运算符重载
        bool operator!=(const Vector3& other) const
        {
            return !(this->X == other.X && this->Y == other.Y && this->Z == other.Z);
        }
        // 加法运算符重载
        Vector3 operator+(const Vector3& right) const
        {
            return Vector3(this->X + right.X, this->Y + right.Y, this->Z + right.Z);
        }
        // 减法运算符重载
        Vector3 operator-(const Vector3& right) const
        {
            return Vector3(this->X - right.X, this->Y - right.Y, this->Z - right.Z);
        }
        // 浮点乘法运算符重载
        Vector3 operator*(const float& other) const
        {
            return Vector3(this->X * other, this->Y * other, this->Z * other);
        }
        // 浮点除法运算符重载
        Vector3 operator/(const float& other) const
        {
            return Vector3(this->X / other, this->Y / other, this->Z / other);
        }

        // 位置变量
        float X;
        float Y;
        float Z;
    };

    // Structure: Vertex 顶点
    //
    // Description: 模型顶点对象，包含位置、法向量、纹理坐标、切线、副切线
    struct Vertex
    {
        // 位置向量
        Vector3 Position;

        // 法向量
        Vector3 Normal;

        // 纹理坐标
        Vector2 TextureCoordinate;

        // 切线向量
        Vector3 Tangent;

        // 副切线向量
        Vector3 Bitangent;
    };

    // Structure: Material 材质
    //
    // Description: 材质对象，包含名称、环境光颜色、漫反射颜色、镜面反射颜色、镜面反射指数、折射率、溶解度、光照、环境光纹理、漫反射纹理、镜面反射纹理、镜面高光纹理、镜面高光指数、透明度纹理、凹凸纹理
    struct Material
    {
        Material()
        {
            name;
            Ns = 0.0f;
            Ni = 0.0f;
            d = 0.0f;
            illum = 0;
        }

        // Material Name 材质名称
        std::string name;
        // Ambient Color 环境光颜色
        Vector3 Ka;
        // Diffuse Color 漫反射颜色
        Vector3 Kd;
        // Specular Color 镜面反射颜色
        Vector3 Ks;
        // Specular Exponent 镜面反射指数
        float Ns;
        // Optical Density 折射率
        float Ni;
        // Dissolve 溶解度
        float d;
        // Illumination 光照
        int illum;
        // Ambient Texture Map 环境光纹理
        std::string map_Ka;
        // Diffuse Texture Map 漫反射纹理
        std::string map_Kd;
        // Specular Texture Map 镜面反射纹理
        std::string map_Ks;
        // Specular Hightlight Map 镜面高光纹理
        std::string map_Ns;
        // Alpha Texture Map 透明度纹理
        std::string map_d;
        // Bump Map 凹凸纹理
        std::string map_bump;
    };

    // Structure: Mesh
    //
    // Description: A Simple Mesh Object that holds
    //	a name, a vertex list, and an index list
    struct Mesh
    {
        // 默认构造函数
        Mesh()
        {

        }
        // 变量设置构造函数
        Mesh(std::vector<Vertex>& _Vertices, std::vector<unsigned int>& _Indices)
        {
            Vertices = _Vertices;
            Indices = _Indices;
        }
        // Mesh Name 网格名称
        std::string MeshName;
        // Vertex List 顶点列表
        std::vector<Vertex> Vertices;
        // Index List 索引列表
        std::vector<unsigned int> Indices;

        // Material 材质
        Material MeshMaterial;
    };

    // Namespace: Math 数学
    //
    // Description: 数学命名空间，包含所有需要的数学函数
    namespace math
    {
        // Vector3 叉积
        Vector3 CrossV3(const Vector3 a, const Vector3 b)
        {
            return Vector3(a.Y * b.Z - a.Z * b.Y,
                           a.Z * b.X - a.X * b.Z,
                           a.X * b.Y - a.Y * b.X);
        }

        // Vector3 向量大小计算
        float MagnitudeV3(const Vector3 in)
        {
            return (sqrtf(powf(in.X, 2) + powf(in.Y, 2) + powf(in.Z, 2)));
        }

        // Vector3 点积
        float DotV3(const Vector3 a, const Vector3 b)
        {
            return (a.X * b.X) + (a.Y * b.Y) + (a.Z * b.Z);
        }

        // 两个 Vector3 对象之间的角度
        float AngleBetweenV3(const Vector3 a, const Vector3 b)
        {
            float angle = DotV3(a, b);
            angle /= (MagnitudeV3(a) * MagnitudeV3(b));
            return angle = acosf(angle);
        }

        // 计算 a 在 b 上的投影
        Vector3 ProjV3(const Vector3 a, const Vector3 b)
        {
            Vector3 bn = b / MagnitudeV3(b);
            return bn * DotV3(a, bn);
        }
    }

    // Namespace: Algorithm
    //
    // Description: 算法命名空间，包含所有需要的算法
    namespace algorithm
    {
        // Vector3 乘法运算符重载
        Vector3 operator*(const float& left, const Vector3& right)
        {
            return Vector3(right.X * left, right.Y * left, right.Z * left);
        }

        // 测试 p1 是否在 a 和 b 的同一侧
        bool SameSide(Vector3 p1, Vector3 p2, Vector3 a, Vector3 b)
        {
            Vector3 cp1 = math::CrossV3(b - a, p1 - a);
            Vector3 cp2 = math::CrossV3(b - a, p2 - a);

            if (math::DotV3(cp1, cp2) >= 0)
                return true;
            else
                return false;
        }

        // 生成一个三角形的交叉积法向量
        Vector3 GenTriNormal(Vector3 t1, Vector3 t2, Vector3 t3)
        {
            Vector3 u = t2 - t1;
            Vector3 v = t3 - t1;

            Vector3 normal = math::CrossV3(u,v);

            return normal;
        }

        // 检查一个 Vector3 点是否在一个 3 个 Vector3 三角形内
        bool inTriangle(Vector3 point, Vector3 tri1, Vector3 tri2, Vector3 tri3)
        {
            // 测试一个点是否在一个无限棱柱体内，该棱柱体由三角形勾勒出来。
            bool within_tri_prisim = SameSide(point, tri1, tri2, tri3) && SameSide(point, tri2, tri1, tri3)
                                     && SameSide(point, tri3, tri1, tri2);

            // 如果它不在三角形内，它将永远不会在三角形内
            if (!within_tri_prisim)
                return false;

            // 计算三角形的法向量
            Vector3 n = GenTriNormal(tri1, tri2, tri3);

            // 将点投影到这个法向量上
            Vector3 proj = math::ProjV3(point, n);

            // 如果从三角形到点的距离为 0，则该点在三角形上
            if (math::MagnitudeV3(proj) == 0)
                return true;
            else
                return false;
        }

        // 将一个字符串分割成一个字符串数组，给定一个标记
        inline void split(const std::string &in,
                          std::vector<std::string> &out,
                          std::string token)
        {
            out.clear();

            std::string temp;

            for (int i = 0; i < int(in.size()); i++)
            {
                std::string test = in.substr(i, token.size());

                if (test == token)
                {
                    if (!temp.empty())
                    {
                        out.push_back(temp);
                        temp.clear();
                        i += (int)token.size() - 1;
                    }
                    else
                    {
                        out.push_back("");
                    }
                }
                else if (i + token.size() >= in.size())
                {
                    temp += in.substr(i, token.size());
                    out.push_back(temp);
                    break;
                }
                else
                {
                    temp += in[i];
                }
            }
        }

        // 获取第一个标记后的字符串尾部，可能跟随空格
        inline std::string tail(const std::string &in)
        {
            size_t token_start = in.find_first_not_of(" \t");
            size_t space_start = in.find_first_of(" \t", token_start);
            size_t tail_start = in.find_first_not_of(" \t", space_start);
            size_t tail_end = in.find_last_not_of(" \t");
            if (tail_start != std::string::npos && tail_end != std::string::npos)
            {
                return in.substr(tail_start, tail_end - tail_start + 1);
            }
            else if (tail_start != std::string::npos)
            {
                return in.substr(tail_start);
            }
            return "";
        }

        // 获取第一个标记
        inline std::string firstToken(const std::string &in)
        {
            if (!in.empty())
            {
                size_t token_start = in.find_first_not_of(" \t");
                size_t token_end = in.find_first_of(" \t", token_start);
                if (token_start != std::string::npos && token_end != std::string::npos)
                {
                    return in.substr(token_start, token_end - token_start);
                }
                else if (token_start != std::string::npos)
                {
                    return in.substr(token_start);
                }
            }
            return "";
        }

        // 获取给定索引位置的元素
        template <class T>
        inline const T & getElement(const std::vector<T> &elements, std::string &index)
        {
            int idx = std::stoi(index);
            if (idx < 0)
                idx = int(elements.size()) + idx;
            else
                idx--;
            return elements[idx];
        }
    }

    // Class: Loader
    //
    // Description: OBJ 模型加载器
    class Loader
    {
    public:
        // 默认构造函数
        Loader()
        {

        }
        ~Loader()
        {
            LoadedMeshes.clear();
        }

        // 加载一个文件到加载器中
        //
        // 如果文件加载成功返回 true
        //
        // 如果文件无法找到或无法加载返回 false
        bool LoadFile(std::string Path)
        {
            // 如果文件不是 .obj 文件返回 false
            if (Path.substr(Path.size() - 4, 4) != ".obj")
                return false;


            std::ifstream file(Path);

            if (!file.is_open())
                return false;

            LoadedMeshes.clear();
            LoadedVertices.clear();
            LoadedIndices.clear();

            std::vector<Vector3> Positions;
            std::vector<Vector2> TCoords;
            std::vector<Vector3> Normals;

            std::vector<Vertex> Vertices;
            std::vector<unsigned int> Indices;

            std::vector<std::string> MeshMatNames;

            bool listening = false;
            std::string meshname;

            Mesh tempMesh;

#ifdef OBJL_CONSOLE_OUTPUT
            const unsigned int outputEveryNth = 1000;
            unsigned int outputIndicator = outputEveryNth;
#endif

            std::string curline;
            while (std::getline(file, curline))
            {
#ifdef OBJL_CONSOLE_OUTPUT
                if ((outputIndicator = ((outputIndicator + 1) % outputEveryNth)) == 1)
                {
                    if (!meshname.empty())
                    {
                        std::cout
                                << "\r- " << meshname
                                << "\t| vertices > " << Positions.size()
                                << "\t| texcoords > " << TCoords.size()
                                << "\t| normals > " << Normals.size()
                                << "\t| triangles > " << (Vertices.size() / 3)
                                << (!MeshMatNames.empty() ? "\t| material: " + MeshMatNames.back() : "");
                    }
                }
#endif

                // 生成一个 Mesh 对象或准备创建一个对象
                if (algorithm::firstToken(curline) == "o" || algorithm::firstToken(curline) == "g" || curline[0] == 'g')
                {
                    if (!listening)
                    {
                        listening = true;

                        if (algorithm::firstToken(curline) == "o" || algorithm::firstToken(curline) == "g")
                        {
                            meshname = algorithm::tail(curline);
                        }
                        else
                        {
                            meshname = "unnamed";
                        }
                    }
                    else
                    {
                        // 生成一个 Mesh 对象放入数组中

                        if (!Indices.empty() && !Vertices.empty())
                        {
                            // 创建一个 Mesh 对象
                            tempMesh = Mesh(Vertices, Indices);
                            tempMesh.MeshName = meshname;

                            // Insert Mesh
                            LoadedMeshes.push_back(tempMesh);

                            // Cleanup
                            Vertices.clear();
                            Indices.clear();
                            meshname.clear();

                            meshname = algorithm::tail(curline);
                        }
                        else
                        {
                            if (algorithm::firstToken(curline) == "o" || algorithm::firstToken(curline) == "g")
                            {
                                meshname = algorithm::tail(curline);
                            }
                            else
                            {
                                meshname = "unnamed";
                            }
                        }
                    }
#ifdef OBJL_CONSOLE_OUTPUT
                    std::cout << std::endl;
                    outputIndicator = 0;
#endif
                }
                // 生成一个顶点位置
                if (algorithm::firstToken(curline) == "v")
                {
                    std::vector<std::string> spos;
                    Vector3 vpos;
                    algorithm::split(algorithm::tail(curline), spos, " ");

                    vpos.X = std::stof(spos[0]);
                    vpos.Y = std::stof(spos[1]);
                    vpos.Z = std::stof(spos[2]);

                    Positions.push_back(vpos);
                }
                // 生成一个顶点纹理坐标
                if (algorithm::firstToken(curline) == "vt")
                {
                    std::vector<std::string> stex;
                    Vector2 vtex;
                    algorithm::split(algorithm::tail(curline), stex, " ");

                    vtex.X = std::stof(stex[0]);
                    vtex.Y = std::stof(stex[1]);

                    TCoords.push_back(vtex);
                }
                // 生成一个顶点法向量
                if (algorithm::firstToken(curline) == "vn")
                {
                    std::vector<std::string> snor;
                    Vector3 vnor;
                    algorithm::split(algorithm::tail(curline), snor, " ");

                    vnor.X = std::stof(snor[0]);
                    vnor.Y = std::stof(snor[1]);
                    vnor.Z = std::stof(snor[2]);

                    Normals.push_back(vnor);
                }
                // 生成一个面（顶点和索引）
                if (algorithm::firstToken(curline) == "f")
                {
                    // 生成顶点
                    std::vector<Vertex> vVerts;
                    GenVerticesFromRawOBJ(vVerts, Positions, TCoords, Normals, curline);

                    // 添加顶点
                    for (int i = 0; i < int(vVerts.size()); i++)
                    {
                        Vertices.push_back(vVerts[i]);

                        LoadedVertices.push_back(vVerts[i]);
                    }

                    std::vector<unsigned int> iIndices;

                    VertexTriangluation(iIndices, vVerts);

                    // 添加索引
                    for (int i = 0; i < int(iIndices.size()); i++)
                    {
                        unsigned int indnum = (unsigned int)((Vertices.size()) - vVerts.size()) + iIndices[i];
                        Indices.push_back(indnum);

                        indnum = (unsigned int)((LoadedVertices.size()) - vVerts.size()) + iIndices[i];
                        LoadedIndices.push_back(indnum);

                    }
                }
                // 获取 Mesh 材质名称
                if (algorithm::firstToken(curline) == "usemtl")
                {
                    MeshMatNames.push_back(algorithm::tail(curline));

                    // 如果材质在组内发生变化，创建一个新的 Mesh 对象
                    if (!Indices.empty() && !Vertices.empty())
                    {
                        // 创建一个 Mesh 对象
                        tempMesh = Mesh(Vertices, Indices);
                        tempMesh.MeshName = meshname;
                        int i = 2;
                        while(1) {
                            tempMesh.MeshName = meshname + "_" + std::to_string(i);

                            for (auto &m : LoadedMeshes)
                                if (m.MeshName == tempMesh.MeshName)
                                    continue;
                            break;
                        }

                        // 插入 Mesh
                        LoadedMeshes.push_back(tempMesh);

                        // 清理
                        Vertices.clear();
                        Indices.clear();
                    }

#ifdef OBJL_CONSOLE_OUTPUT
                    outputIndicator = 0;
#endif
                }
                // 加载材质
                if (algorithm::firstToken(curline) == "mtllib")
                {
                    // 生成 LoadedMaterial

                    // 生成一个到材质文件的路径
                    std::vector<std::string> temp;
                    algorithm::split(Path, temp, "/");

                    std::string pathtomat = "";

                    if (temp.size() != 1)
                    {
                        for (int i = 0; i < temp.size() - 1; i++)
                        {
                            pathtomat += temp[i] + "/";
                        }
                    }


                    pathtomat += algorithm::tail(curline);

#ifdef OBJL_CONSOLE_OUTPUT
                    std::cout << std::endl << "- find materials in: " << pathtomat << std::endl;
#endif

                    // 加载材质
                    LoadMaterials(pathtomat);
                }
            }

#ifdef OBJL_CONSOLE_OUTPUT
            std::cout << std::endl;
#endif

            // 处理最后一个 Mesh

            if (!Indices.empty() && !Vertices.empty())
            {
                // 创建一个 Mesh 对象
                tempMesh = Mesh(Vertices, Indices);
                tempMesh.MeshName = meshname;

                // 插入 Mesh
                LoadedMeshes.push_back(tempMesh);
            }

            file.close();

            // 为每个 Mesh 设置材质
            for (int i = 0; i < MeshMatNames.size(); i++)
            {
                std::string matname = MeshMatNames[i];

                // 在加载的材质中找到对应的材质名称
                // 当找到时，将材质变量复制到 Mesh 材质中
                for (int j = 0; j < LoadedMaterials.size(); j++)
                {
                    if (LoadedMaterials[j].name == matname)
                    {
                        LoadedMeshes[i].MeshMaterial = LoadedMaterials[j];
                        break;
                    }
                }
            }

            if (LoadedMeshes.empty() && LoadedVertices.empty() && LoadedIndices.empty())
            {
                return false;
            }
            else
            {
                return true;
            }
        }

        // 加载的 Mesh 对象
        std::vector<Mesh> LoadedMeshes;
        // 加载的顶点对象
        std::vector<Vertex> LoadedVertices;
        // 加载的索引位置
        std::vector<unsigned int> LoadedIndices;
        // 加载的材质对象
        std::vector<Material> LoadedMaterials;

    private:
        // 从列表中生成顶点，位置，纹理坐标，法向量和面线
        void GenVerticesFromRawOBJ(std::vector<Vertex>& oVerts,
                                   const std::vector<Vector3>& iPositions,
                                   const std::vector<Vector2>& iTCoords,
                                   const std::vector<Vector3>& iNormals,
                                   std::string icurline)
        {
            std::vector<std::string> sface, svert;
            Vertex vVert;
            algorithm::split(algorithm::tail(icurline), sface, " ");

            bool noNormal = false;

            // 对于每个给定的顶点执行此操作
            for (int i = 0; i < int(sface.size()); i++)
            {
                // 查看顶点的类型
                int vtype;

                algorithm::split(sface[i], svert, "/");

                // 检查是否只有位置 - v1
                if (svert.size() == 1)
                {
                    // 只有位置
                    vtype = 1;
                }

                // 检查是否只有位置和纹理 - v1/vt1
                if (svert.size() == 2)
                {
                    // 位置和纹理
                    vtype = 2;
                }

                // 检查是否只有位置，纹理和法向量 - v1/vt1/vn1
                // 或者如果只有位置和法向量 - v1//vn1
                if (svert.size() == 3)
                {
                    if (svert[1] != "")
                    {
                        // 位置，纹理，和法向量
                        vtype = 4;
                    }
                    else
                    {
                        // 位置和法向量
                        vtype = 3;
                    }
                }

                // 计算并存储顶点
                switch (vtype)
                {
                    case 1: // P
                    {
                        vVert.Position = algorithm::getElement(iPositions, svert[0]);
                        vVert.TextureCoordinate = Vector2(0, 0);
                        noNormal = true;
                        oVerts.push_back(vVert);
                        break;
                    }
                    case 2: // P/T
                    {
                        vVert.Position = algorithm::getElement(iPositions, svert[0]);
                        vVert.TextureCoordinate = algorithm::getElement(iTCoords, svert[1]);
                        noNormal = true;
                        oVerts.push_back(vVert);
                        break;
                    }
                    case 3: // P//N
                    {
                        vVert.Position = algorithm::getElement(iPositions, svert[0]);
                        vVert.TextureCoordinate = Vector2(0, 0);
                        vVert.Normal = algorithm::getElement(iNormals, svert[2]);
                        oVerts.push_back(vVert);
                        break;
                    }
                    case 4: // P/T/N
                    {
                        vVert.Position = algorithm::getElement(iPositions, svert[0]);
                        vVert.TextureCoordinate = algorithm::getElement(iTCoords, svert[1]);
                        vVert.Normal = algorithm::getElement(iNormals, svert[2]);
                        oVerts.push_back(vVert);
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
            }

            // 处理缺失的法向量
            // 这些可能不准确，但这是没有编译带有法向量的网格的最佳方法
            if (noNormal)
            {
                Vector3 A = oVerts[0].Position - oVerts[1].Position;
                Vector3 B = oVerts[2].Position - oVerts[1].Position;

                Vector3 normal = math::CrossV3(A, B);

                for (int i = 0; i < int(oVerts.size()); i++)
                {
                    oVerts[i].Normal = normal;
                }
            }
        }

        // 将顶点列表三角化成一个面，通过打印
        //	inducies 对应于其中的三角形
        void VertexTriangluation(std::vector<unsigned int>& oIndices,
                                 const std::vector<Vertex>& iVerts)
        {
            // 如果顶点少于3个，
            // 不能创建三角形，
            // 所以退出
            if (iVerts.size() < 3)
            {
                return;
            }
            // 如果是一个三角形，不需要计算
            if (iVerts.size() == 3)
            {
                oIndices.push_back(0);
                oIndices.push_back(1);
                oIndices.push_back(2);
                return;
            }

            // 创建一个顶点列表
            std::vector<Vertex> tVerts = iVerts;

            while (true)
            {
                // 对于每个顶点
                for (int i = 0; i < int(tVerts.size()); i++)
                {
                    // pPrev = 列表中的前一个顶点
                    Vertex pPrev;
                    if (i == 0)
                    {
                        pPrev = tVerts[tVerts.size() - 1];
                    }
                    else
                    {
                        pPrev = tVerts[i - 1];
                    }

                    // pCur = 当前顶点
                    Vertex pCur = tVerts[i];

                    // pNext = 列表中的下一个顶点
                    Vertex pNext;
                    if (i == tVerts.size() - 1)
                    {
                        pNext = tVerts[0];
                    }
                    else
                    {
                        pNext = tVerts[i + 1];
                    }

                    // 检查是否只剩下3个顶点
                    // 如果是，这是最后一个三角形
                    if (tVerts.size() == 3)
                    {
                        // 从 pCur, pPrev, pNext 创建一个三角形
                        for (int j = 0; j < int(tVerts.size()); j++)
                        {
                            if (iVerts[j].Position == pCur.Position)
                                oIndices.push_back(j);
                            if (iVerts[j].Position == pPrev.Position)
                                oIndices.push_back(j);
                            if (iVerts[j].Position == pNext.Position)
                                oIndices.push_back(j);
                        }

                        tVerts.clear();
                        break;
                    }
                    if (tVerts.size() == 4)
                    {
                        // 从 pCur, pPrev, pNext 创建一个三角形
                        for (int j = 0; j < int(iVerts.size()); j++)
                        {
                            if (iVerts[j].Position == pCur.Position)
                                oIndices.push_back(j);
                            if (iVerts[j].Position == pPrev.Position)
                                oIndices.push_back(j);
                            if (iVerts[j].Position == pNext.Position)
                                oIndices.push_back(j);
                        }

                        Vector3 tempVec;
                        for (int j = 0; j < int(tVerts.size()); j++)
                        {
                            if (tVerts[j].Position != pCur.Position
                                && tVerts[j].Position != pPrev.Position
                                && tVerts[j].Position != pNext.Position)
                            {
                                tempVec = tVerts[j].Position;
                                break;
                            }
                        }

                        // 从 pCur, pPrev, pNext 创建一个三角形
                        for (int j = 0; j < int(iVerts.size()); j++)
                        {
                            if (iVerts[j].Position == pPrev.Position)
                                oIndices.push_back(j);
                            if (iVerts[j].Position == pNext.Position)
                                oIndices.push_back(j);
                            if (iVerts[j].Position == tempVec)
                                oIndices.push_back(j);
                        }

                        tVerts.clear();
                        break;
                    }

                    // 如果顶点不是内部顶点
                    float angle = math::AngleBetweenV3(pPrev.Position - pCur.Position, pNext.Position - pCur.Position) * (180 / 3.14159265359);
                    if (angle <= 0 && angle >= 180)
                        continue;

                    // 如果任何顶点在这个三角形内
                    bool inTri = false;
                    for (int j = 0; j < int(iVerts.size()); j++)
                    {
                        if (algorithm::inTriangle(iVerts[j].Position, pPrev.Position, pCur.Position, pNext.Position)
                            && iVerts[j].Position != pPrev.Position
                            && iVerts[j].Position != pCur.Position
                            && iVerts[j].Position != pNext.Position)
                        {
                            inTri = true;
                            break;
                        }
                    }
                    if (inTri)
                        continue;

                    // 从 pCur, pPrev, pNext 创建一个三角形
                    for (int j = 0; j < int(iVerts.size()); j++)
                    {
                        if (iVerts[j].Position == pCur.Position)
                            oIndices.push_back(j);
                        if (iVerts[j].Position == pPrev.Position)
                            oIndices.push_back(j);
                        if (iVerts[j].Position == pNext.Position)
                            oIndices.push_back(j);
                    }

                    // 从列表中删除 pCur
                    for (int j = 0; j < int(tVerts.size()); j++)
                    {
                        if (tVerts[j].Position == pCur.Position)
                        {
                            tVerts.erase(tVerts.begin() + j);
                            break;
                        }
                    }

                    // 重置 i 到开始
                    // -1 因为循环会加1到它
                    i = -1;
                }

                // 如果没有创建三角形
                if (oIndices.size() == 0)
                    break;

                // 如果没有更多的顶点
                if (tVerts.size() == 0)
                    break;
            }
        }

        // 从 .mtl 文件加载材质
        bool LoadMaterials(std::string path)
        {
            // 如果文件不是材质文件返回 false
            if (path.substr(path.size() - 4, path.size()) != ".mtl")
                return false;

            std::ifstream file(path);

            // 如果文件未找到返回 false
            if (!file.is_open())
                return false;

            Material tempMaterial;

            bool listening = false;

            // 遍历每一行，寻找材质变量
            std::string curline;
            while (std::getline(file, curline))
            {
                // 新材质和材质名称
                if (algorithm::firstToken(curline) == "newmtl")
                {
                    if (!listening)
                    {
                        listening = true;

                        if (curline.size() > 7)
                        {
                            tempMaterial.name = algorithm::tail(curline);
                        }
                        else
                        {
                            tempMaterial.name = "none";
                        }
                    }
                    else
                    {
                        // 生成材质

                        // 推回加载的材质
                        LoadedMaterials.push_back(tempMaterial);

                        // 清除加载的材质
                        tempMaterial = Material();

                        if (curline.size() > 7)
                        {
                            tempMaterial.name = algorithm::tail(curline);
                        }
                        else
                        {
                            tempMaterial.name = "none";
                        }
                    }
                }
                // 环境颜色
                if (algorithm::firstToken(curline) == "Ka")
                {
                    std::vector<std::string> temp;
                    algorithm::split(algorithm::tail(curline), temp, " ");

                    if (temp.size() != 3)
                        continue;

                    tempMaterial.Ka.X = std::stof(temp[0]);
                    tempMaterial.Ka.Y = std::stof(temp[1]);
                    tempMaterial.Ka.Z = std::stof(temp[2]);
                }
                // 漫反射颜色
                if (algorithm::firstToken(curline) == "Kd")
                {
                    std::vector<std::string> temp;
                    algorithm::split(algorithm::tail(curline), temp, " ");

                    if (temp.size() != 3)
                        continue;

                    tempMaterial.Kd.X = std::stof(temp[0]);
                    tempMaterial.Kd.Y = std::stof(temp[1]);
                    tempMaterial.Kd.Z = std::stof(temp[2]);
                }
                // 镜面颜色
                if (algorithm::firstToken(curline) == "Ks")
                {
                    std::vector<std::string> temp;
                    algorithm::split(algorithm::tail(curline), temp, " ");

                    if (temp.size() != 3)
                        continue;

                    tempMaterial.Ks.X = std::stof(temp[0]);
                    tempMaterial.Ks.Y = std::stof(temp[1]);
                    tempMaterial.Ks.Z = std::stof(temp[2]);
                }
                // 镜面指数
                if (algorithm::firstToken(curline) == "Ns")
                {
                    tempMaterial.Ns = std::stof(algorithm::tail(curline));
                }
                // 光学密度
                if (algorithm::firstToken(curline) == "Ni")
                {
                    tempMaterial.Ni = std::stof(algorithm::tail(curline));
                }
                // 溶解
                if (algorithm::firstToken(curline) == "d")
                {
                    tempMaterial.d = std::stof(algorithm::tail(curline));
                }
                // 光照
                if (algorithm::firstToken(curline) == "illum")
                {
                    tempMaterial.illum = std::stoi(algorithm::tail(curline));
                }
                // 环境纹理图
                if (algorithm::firstToken(curline) == "map_Ka")
                {
                    tempMaterial.map_Ka = algorithm::tail(curline);
                }
                // 漫反射纹理图
                if (algorithm::firstToken(curline) == "map_Kd")
                {
                    tempMaterial.map_Kd = algorithm::tail(curline);
                }
                // 镜面纹理图
                if (algorithm::firstToken(curline) == "map_Ks")
                {
                    tempMaterial.map_Ks = algorithm::tail(curline);
                }
                // Specular Hightlight Map
                if (algorithm::firstToken(curline) == "map_Ns")
                {
                    tempMaterial.map_Ns = algorithm::tail(curline);
                }
                // Alpha Texture Map
                if (algorithm::firstToken(curline) == "map_d")
                {
                    tempMaterial.map_d = algorithm::tail(curline);
                }
                // Bump Map
                if (algorithm::firstToken(curline) == "map_Bump" || algorithm::firstToken(curline) == "map_bump" || algorithm::firstToken(curline) == "bump")
                {
                    tempMaterial.map_bump = algorithm::tail(curline);
                }
            }

            // Deal with last material

            // Push Back loaded Material
            LoadedMaterials.push_back(tempMaterial);

            // Test to see if anything was loaded
            // If not return false
            if (LoadedMaterials.empty())
                return false;
                // If so return true
            else
                return true;
        }
    };
}
#endif //RASTERIZER_OBJ_LOADER_H
