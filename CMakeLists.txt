# 设置最低的 CMake 版本
cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(yuvbmp_Con)

# 设置输出目录为 bin
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

# 添加可执行文件并指定源文件
add_executable(yuv2bmp yuv2bmp.c)
add_executable(bmp2yuv bmp2yuv.c)