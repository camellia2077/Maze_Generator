#!/bin/bash

# 这是一个在 MSYS64 环境下为 MazeGenerator 项目自动化构建过程的脚本。
# 它会通过移除旧的构建目录来确保一个干净的构建。

# 如果任何命令执行失败，立即退出脚本
set -e

# 定义构建目录的名称
BUILD_DIR="build"

# 检查构建目录是否存在，如果存在则删除它
if [ -d "$BUILD_DIR" ]; then
    echo "正在删除已存在的 build 目录..."
    rm -rf "$BUILD_DIR"
fi

# 创建一个新的构建目录并进入该目录
echo "正在创建 build 目录..."
mkdir "$BUILD_DIR"
cd "$BUILD_DIR"

# 运行 CMake 来配置项目并生成 Makefile
# 我们为 MSYS64 环境指定 "MSYS Makefiles" 作为生成器。
# 通过 -DCMAKE_BUILD_TYPE=Release 开启最高优化
echo "正在使用 CMake 配置项目 (Release 模式)..."
cmake -G "MSYS Makefiles" -DCMAKE_BUILD_TYPE=Release ..

# 使用 make 命令编译项目
echo "正在使用 make 构建项目..."
make

# 显示构建完成后的提示信息
echo ""
echo "构建完成!"
# 根据 CMakeLists.txt 文件，可执行文件名为 maze_generator_app
echo "可执行文件 'maze_generator_app.exe' 已生成在 '$BUILD_DIR' 目录中。"
echo "你可以从项目根目录运行它: ./$BUILD_DIR/maze_generator_app"