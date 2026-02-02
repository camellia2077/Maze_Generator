# --- Targets ---
add_executable(maze_generator_app
    src/main.cpp
    src/cli/framework/cli_app.cpp
    src/cli/commands/generation_algorithms_command.cpp
    src/cli/commands/search_algorithms_command.cpp
    src/cli/commands/version_command.cpp
    src/common/pch.cpp
    src/infrastructure/config/config_loader.cpp
    src/domain/maze_generation.cpp
    src/domain/maze_solver_common.cpp
    src/domain/maze_solver.cpp
    src/domain/maze_solver_bfs.cpp
    src/domain/maze_solver_dfs.cpp
    src/domain/maze_solver_astar.cpp
    src/domain/maze_solver_dijkstra.cpp
    src/domain/maze_solver_greedy.cpp
    src/application/services/maze_generation.cpp
    src/application/services/maze_solver.cpp
    src/infrastructure/graphics/maze_renderer.cpp
)

target_precompile_headers(maze_generator_app PRIVATE
    src/common/pch.h
)

target_include_directories(maze_generator_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(maze_generator_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor
)

target_include_directories(maze_generator_app SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/stb
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(maze_generator_app PRIVATE -Wall -Wpedantic)
elseif(MSVC)
    target_compile_options(maze_generator_app PRIVATE /W4)
endif()
