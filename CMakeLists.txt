cmake_minimum_required(VERSION 3.14)
project(MazeGenerator LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- 添加 Release 模式下的编译和链接优化选项 ---
# -O3: 最高等级优化
# -march=native: 为本机CPU优化
# -flto: 开启链接时优化
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto")
# -s: 移除符号表，减小文件体积
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")

# 定义可执行文件及其源文件
# 源文件现在位于 src/ 目录下
add_executable(maze_generator_app
    src/maze_generator.cpp
    src/config_loader.cpp
    src/maze_generation.cpp
    src/maze_solver.cpp
)

# --- 修改此处以包含头文件目录 ---
# 将 src 目录添加为常规包含目录，以便项目内部的头文件可以被找到
target_include_directories(maze_generator_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 将第三方库 lib/stb 目录标记为 SYSTEM，编译器会忽略来自该目录的警告
target_include_directories(maze_generator_app SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/stb
)

# 为项目开启常规警告（这些警告不会应用到上面的 SYSTEM 目录）
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(maze_generator_app PRIVATE -Wall -Wpedantic)
elseif(MSVC)
    target_compile_options(maze_generator_app PRIVATE /W4)
endif()

# 显示配置信息
message(STATUS "Configured MazeGenerator project.")
message(STATUS "Build with your chosen generator.")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")