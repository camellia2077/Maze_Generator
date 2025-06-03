#include <iostream>
#include <vector>
#include <string>
#include <random> 
#include "maze_generation_module.h"
#include "config_loader.h"
#include "maze_solver.h" 


int main() {
    ConfigLoader::load_config("config.ini");

    if (ConfigLoader::MAZE_WIDTH <= 0 || ConfigLoader::MAZE_HEIGHT <= 0) {
        std::cerr << "Error: MAZE_WIDTH and MAZE_HEIGHT must be greater than 0. Exiting." << std::endl;
        return 1;
    }
    if (ConfigLoader::UNIT_PIXELS <= 0) {
        std::cerr << "Error: UNIT_PIXELS must be greater than 0. Exiting." << std::endl;
        return 1;
    }
    if (ConfigLoader::ACTIVE_GENERATION_ALGORITHMS.empty()) {
        std::cerr << "Error: No maze generation algorithms selected after loading config. Exiting." << std::endl;
        return 1;
    }

    std::vector<std::vector<MazeGeneration::GenCell>> generation_maze_data;
    // Use the Cell type from the MazeSolver namespace
    std::vector<std::vector<MazeSolver::Cell>> solver_maze_data;

    for (const auto& algo_info : ConfigLoader::ACTIVE_GENERATION_ALGORITHMS) {
        // The global ConfigLoader::CURRENT_GENERATION_ALGORITHM_NAME_FOR_FOLDER is no longer
        // strictly needed by the solver functions as the name is passed directly.
        // You can still set it if other parts of your system rely on it, or remove its usage.
        // For this refactoring, we pass algo_info.name to the solver functions.
        // ConfigLoader::CURRENT_GENERATION_ALGORITHM_NAME_FOR_FOLDER = algo_info.name;


        std::cout << "\n--- Processing for Maze Generation Algorithm: "
                  << algo_info.name << " ---" << std::endl;

        generation_maze_data.assign(ConfigLoader::MAZE_HEIGHT, std::vector<MazeGeneration::GenCell>(ConfigLoader::MAZE_WIDTH));
        // Initialize solver_maze_data with the correct dimensions.
        // MazeSolver::Cell members (walls, parent) will be default-initialized.
        // 'parent' will be {-1,-1} by default, which is what solvers expect initially.
        solver_maze_data.assign(ConfigLoader::MAZE_HEIGHT, std::vector<MazeSolver::Cell>(ConfigLoader::MAZE_WIDTH));


        std::cout << "Generating maze structure..." << std::endl;
        int gen_start_r = (ConfigLoader::START_NODE.first >= 0 && ConfigLoader::START_NODE.first < ConfigLoader::MAZE_HEIGHT) ? ConfigLoader::START_NODE.first : 0;
        int gen_start_c = (ConfigLoader::START_NODE.second >= 0 && ConfigLoader::START_NODE.second < ConfigLoader::MAZE_WIDTH) ? ConfigLoader::START_NODE.second : 0;

        if (!(ConfigLoader::START_NODE.first >= 0 && ConfigLoader::START_NODE.first < ConfigLoader::MAZE_HEIGHT &&
              ConfigLoader::START_NODE.second >= 0 && ConfigLoader::START_NODE.second < ConfigLoader::MAZE_WIDTH)) {
             if (algo_info.type == MazeGeneration::MazeAlgorithmType::DFS || algo_info.type == MazeGeneration::MazeAlgorithmType::PRIMS) {
                std::cout << "Adjusted maze generation start point to (" << gen_start_r << "," << gen_start_c
                          << ") due to out-of-bounds config START_NODE for DFS/Prims." << std::endl;
             }
        }

        MazeGeneration::generate_maze_structure(generation_maze_data, gen_start_r, gen_start_c, ConfigLoader::MAZE_WIDTH, ConfigLoader::MAZE_HEIGHT, algo_info.type);

        // Transfer wall data from generation_maze_data to solver_maze_data
        for (int r = 0; r < ConfigLoader::MAZE_HEIGHT; ++r) {
            for (int c = 0; c < ConfigLoader::MAZE_WIDTH; ++c) {
                for (int i = 0; i < 4; ++i) {
                    solver_maze_data[r][c].walls[i] = generation_maze_data[r][c].walls[i];
                }
                // solver_maze_data[r][c].parent is already initialized to {-1,-1} by default
                // or will be reset within the solver functions.
            }
        }
        std::cout << "Maze generated and data transferred." << std::endl;

        // Call solver functions from the MazeSolver namespace, passing the solver_maze_data
        // and the algorithm name for folder creation.
        MazeSolver::solve_bfs(solver_maze_data, algo_info.name);
        MazeSolver::solve_dfs(solver_maze_data, algo_info.name);
    }

    std::cout << "\nProcessing complete for all selected algorithms. Images saved in respective folders." << std::endl;

    return 0;
}