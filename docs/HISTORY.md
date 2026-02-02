# 2026-02-02 - v0.2.1
- 统一构建入口：sh 脚本仅转发参数，build.py 支持 target/build-type，并为 clang-tidy 提供逐文件实时输出与 -fix。
- CMake 顶层开启编译数据库，clang-tidy 读取 compile_commands.json；检查范围限定在 src 并排除第三方目录。
- 降低复杂度：main/cli_app/maze_solver/maze_renderer 拆分为小函数与流程编排。
- 规整参数与命名：引入结构体封装同类型参数、替换魔数、修正短变量名与返回初始化风格。

# 2026-02-01 - v0.2.0
- Unified maze model into domain MazeGrid/MazeCell; removed wall data copy between generator/solver.
- Split solver into domain algorithms (BFS/DFS/A*) + SearchResult/frames, with separate renderer.
- Updated CLI: run command, -v/--version, --generation-algorithms override, help lists supported algorithms.
- Config loading now uses exe-dir `config/config.toml`.
- CMake refactor into cmake/ modules, with tooling targets for clang-format/clang-tidy.
- Added scripts: format.sh, build_tidy.sh (build_debug), and build.py optimization toggle.

# 2026-02-01 - v0.1.0
- Added domain-layer maze generation with strategy factory + metadata (algorithm names, parsing).
- Introduced Direction enum + wall carving helpers to reduce magic indices.
- Decoupled TOML parsing from runtime config via AppConfig structs.
- Added CLI framework with commands: version, generation-algorithms, help; added --output/-o.
- Added version definition in `common/version.hpp`.
