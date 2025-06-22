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
add_executable(maze_generator_app
    maze_generator.cpp
    config_loader.cpp
    maze_generation.cpp
    maze_solver.cpp
)

# --- 修改此处以抑制第三方库的警告 ---
# 将包含 stb_image_write.h 的目录标记为 SYSTEM，编译器会忽略来自系统目录的警告
target_include_directories(maze_generator_app SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 为项目开启常规警告（这些警告不会应用到上面的 SYSTEM 目录）
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(maze_generator_app PRIVATE -Wall -Wpedantic)
elseif(MSVC)
    target_compile_options(maze_generator_app PRIVATE /W4)
endif()

# 显示配置信息
message(STATUS "Configured MazeGenerator project. Build with your chosen generator.")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")