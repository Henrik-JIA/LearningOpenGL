cmake_minimum_required(VERSION 3.10)
project(LearningOpenGL CXX)

# 设置编译选项
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -g)

# 设置MSYS2库路径
set(MSYS2_PREFIX "D:/msys64/mingw64")
# 查找Assimp库
find_package(assimp REQUIRED)
include_directories(${ASSIMP_INCLUDE_DIRS})

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output)

# 包含头文件目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${MSYS2_PREFIX}/include
)

# 添加库目录
link_directories(lib)

# 添加源文件（必须包含所有核心文件）
file(GLOB SOURCES
    ${CMAKE_SOURCE_DIR}/include/imgui/*.cpp
)

# 添加可执行文件
add_executable(
    main src_raytracing/03_Raytracing_06/main.cpp
    ${SOURCES}
)
# main src_rasterization/19_Framebuffers_03/main.cpp
# main src_raytracing/03_Raytracing_05/main.cpp
# main src_cornell_box/main.cpp

# 链接第三方库
target_link_libraries(main
    glad
    glfw3dll
    opengl32
    ${ASSIMP_LIBRARIES}
    ${MSYS2_PREFIX}/lib/libz.dll.a
)

# 添加清理目标
add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/output
    COMMENT "Cleaning all build artifacts"
)

# 添加运行目标
add_custom_target(run
    COMMAND ./main
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    DEPENDS main
)

