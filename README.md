# Maze_Generator
这是一个 C++ 项目，能够通过多种算法生成迷宫，并使用BFS和DFS对生成的迷宫进行求解。项目会为每个生成算法和求解过程生成可视化帧图片，展示迷宫的构建和路径的寻找过程。图片生成使用了stb_image_write.h库
## 依赖项 (Dependencies)

本项目使用了以下第三方库：

* [stb_image_write.h](https://github.com/nothings/stb/blob/master/stb_image_write.h) - 用于图像写入。
## 功能特性

* **多种迷宫生成算法**:
  深度优先搜索 (DFS - Recursive Backtracker)
  Prim 算法 (Randomized Prim's Algorithm)
   Kruskal 算法 (Randomized Kruskal's Algorithm)
* **两种迷宫求解算法**:
  广度优先搜索 (BFS) - 保证找到最短路径
  深度优先搜索 (DFS)
* **可配置性**:
  通过 `config.ini` 文件配置迷宫的宽度、高度、单元格像素大小。
  可配置迷宫的起点和终点。
  可选择一个或多个迷宫生成算法进行处理。
  可自定义迷宫及求解过程可视化中使用的各种颜色。
* **可视化输出**:
  为每个迷宫生成算法的每个求解过程（BFS 和 DFS）生成一系列 PNG 图片帧，保存在独立的文件夹中。
  图片帧清晰展示了迷宫墙壁、路径、搜索边界、当前处理单元格以及最终找到的解决方案。
  优化了帧保存，对于直线路径段会跳过部分帧以减少图片数量。
* **模块化设计**:
  **`config_loader`**: 负责解析 `config.ini` 配置文件。
  **`maze_generation_module`**: 包含迷宫生成算法的逻辑。
  **`maze_solver`**: 包含迷宫求解算法 (BFS, DFS) 及图像保存逻辑。
  **`maze_generator` (main)**: 协调各个模块，处理程序流程。

## 文件结构
├── maze_generator                # 编译后的可执行文件

├── maze_generator.cpp            # 主程序文件

├── maze_solver.h                 # 迷宫求解器头文件

├── maze_solver.cpp               # 迷宫求解器实现文件

├── maze_generation_module.h      # 迷宫生成模块头文件

├── maze_generation_module.cpp    # 迷宫生成模块实现文件

├── config_loader.h               # 配置加载器头文件

├── config_loader.cpp             # 配置加载器实现文件

├── stb_image_write.h             # 用于图像输出的单头文件库

├── config.ini                    # 配置文件示例

├── bfs_frames_generated_by_XXX/  # BFS 求解过程的输出帧文件夹

└── dfs_frames_generated_by_XXX/  # DFS 求解过程的输出帧文件夹

## 如何编译
g++ -std=c++17 -O3 maze_generator.cpp config_loader.cpp maze_generation_module.cpp maze_solver.cpp -o maze_generator -lstdc++fs
