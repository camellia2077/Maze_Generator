# CMake 3.16 是 target_precompile_headers 的最低版本要求
cmake_minimum_required(VERSION 3.16)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Enable compilation database" FORCE)
project(MazeGenerator LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/Options.cmake)
include(cmake/Compiler.cmake)
include(cmake/Targets.cmake)
include(cmake/PostBuild.cmake)
include(cmake/Messages.cmake)
include(cmake/Tools.cmake)
