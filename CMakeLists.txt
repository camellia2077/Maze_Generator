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
add_executable(maze_generator_app
    src/main.cpp
    src/common/pch.cpp
    src/config/config_loader.cpp
    src/generation/maze_generation.cpp
    src/solver/maze_solver.cpp
)

# --- 启用预编译头文件 (PCH) ---
target_precompile_headers(maze_generator_app PRIVATE
    src/common/pch.h
)

# --- 添加头文件目录 ---
# 将 src 目录作为根包含目录，以支持 "module/header.h" 形式的绝对路径引用
target_include_directories(maze_generator_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# (*** 新增 ***)
# 将我们存放第三方库的 vendor 目录添加进来
target_include_directories(maze_generator_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor
)

# 将第三方库 stb 目录标记为 SYSTEM
target_include_directories(maze_generator_app SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/stb
)

# 为项目开启常规警告
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(maze_generator_app PRIVATE -Wall -Wpedantic)
elseif(MSVC)
    target_compile_options(maze_generator_app PRIVATE /W4)
endif()

# --- 构建后复制配置文件 (已更新) ---
# 这个命令会在构建完成后，将 config.toml 文件复制到可执行文件所在的目录
add_custom_command(
    TARGET maze_generator_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            # (*** 此处已修改 ***)
            ${CMAKE_CURRENT_SOURCE_DIR}/config/config.toml
            $<TARGET_FILE_DIR:maze_generator_app>
    COMMENT "Copying config.toml to output directory"
)

# 显示配置信息
message(STATUS "Configured MazeGenerator project.")
message(STATUS "Build with your chosen generator.")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")