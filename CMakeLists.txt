# CMake 3.16 是 target_precompile_headers 的最低版本要求
cmake_minimum_required(VERSION 3.16)
project(MazeGenerator LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- 添加 Release 模式下的编译和链接优化选项 ---
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")

# --- 启用 ccache ---
# 在系统中查找 ccache 程序
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    # 如果找到，就将其设置为 C++ 编译器的启动器
    set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
    message(STATUS "ccache found, will be used for compilation.")
else()
    message(STATUS "ccache not found, proceeding without it.")
endif()

# 定义可执行文件及其源文件
# 新增了 src/pch.cpp 用于生成预编译头文件
add_executable(maze_generator_app
    src/maze_generator.cpp
    src/config_loader.cpp
    src/maze_generation.cpp
    src/maze_solver.cpp
    src/pch.cpp
)

# --- 启用预编译头文件 (PCH) ---
# 将 src/pch.h 指定为目标 maze_generator_app 的预编译头文件
# CMake 会自动处理 PCH 的创建和使用，并将其包含到所有源文件中
target_precompile_headers(maze_generator_app PRIVATE
    src/pch.h
)

# --- 添加头文件目录 ---
# 将 src 目录添加为常规包含目录
target_include_directories(maze_generator_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 将第三方库 lib 目录标记为 SYSTEM
target_include_directories(maze_generator_app SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/stb
)

# 为项目开启常规警告
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(maze_generator_app PRIVATE -Wall -Wpedantic)
elseif(MSVC)
    target_compile_options(maze_generator_app PRIVATE /W4)
endif()

# 显示配置信息
message(STATUS "Configured MazeGenerator project.")
message(STATUS "Build with your chosen generator.")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
