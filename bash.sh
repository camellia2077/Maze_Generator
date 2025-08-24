#!/bin/bash

# 这是一个在 MSYS64 环境下为 MazeGenerator 项目自动化构建过程的脚本。
# 它会自动切换到脚本所在的目录，确保可以在任何路径下正确执行。
# 编译完成后，可执行文件会保留在 build 目录中。

# 如果任何命令执行失败，立即退出脚本
set -e

# --- 关键步骤: 切换到脚本文件所在的目录 ---
# 这确保了无论从哪里调用此脚本，它总是在正确的项目根目录中运行。
cd "$(dirname "$0")"
echo "当前工作目录已切换到: $(pwd)"

# 定义构建目录和可执行文件的名称
BUILD_DIR="build"
EXECUTABLE_NAME="maze_generator_app.exe"

# --- 第1步: 清理 ---
# 检查构建目录是否存在，如果存在则删除它以进行干净的构建
if [ -d "$BUILD_DIR" ]; then
    echo "正在删除已存在的 build 目录..."
    rm -rf "$BUILD_DIR"
fi

# --- 第2步: 配置 ---
# 创建一个新的构建目录并进入该目录
echo "正在创建 build 目录..."
mkdir "$BUILD_DIR"
cd "$BUILD_DIR"

# 运行 CMake 来配置项目并生成 Makefile
# 我们为 MSYS64 环境指定 "MSYS Makefiles" 作为生成器。
# 通过 -DCMAKE_BUILD_TYPE=Release 开启最高优化
echo "正在使用 CMake 配置项目 (Release 模式)..."
cmake -G "MSYS Makefiles" -DCMAKE_BUILD_TYPE=Release ..

# --- 第3步: 构建 ---
# 使用 make 命令编译项目
echo "正在使用 make 构建项目..."
make

# --- 第4步: 显示最终信息 ---
# 返回到项目根目录
cd ..
echo ""
echo "构建完成!"
echo "可执行文件 '$EXECUTABLE_NAME' 已生成在 '$BUILD_DIR' 目录中。"
echo "你可以运行它: ./$BUILD_DIR/$EXECUTABLE_NAME"
