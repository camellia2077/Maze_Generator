#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <chrono> // Required for high-precision timing
#include <iomanip> // Required for std::fixed and std::setprecision

#include "generation/maze_generation.h"
#include "config/config_loader.h"
#include "solver/maze_solver.h"

// ANSI escape codes for colors
const std::string RESET_COLOR = "\033[0m";
const std::string GREEN_COLOR = "\033[32m";

int main() {
    std::cout << "--- Parameter Loading ---" << std::endl;
    auto start_time_config = std::chrono::high_resolution_clock::now();
    
    // 修改这里，加载 config.toml
    ConfigLoader::load_config("config.toml"); 
    
    auto end_time_config = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_taken_config = end_time_config - start_time_config; // Duration in seconds
    std::cout << GREEN_COLOR << std::fixed << std::setprecision(3) << "Time to load config: " << time_taken_config.count() << " s" << RESET_COLOR << std::endl;

    // ... (main 函数的其余部分保持不变) ...

    std::vector<std::vector<MazeGeneration::GenCell>> generation_maze_data; 
    std::vector<std::vector<MazeSolver::Cell>> solver_maze_data; 

    for (const auto& algo_info : ConfigLoader::ACTIVE_GENERATION_ALGORITHMS) { 
        std::cout << "\n--- Processing for Maze Generation Algorithm: "
                  << algo_info.name << " ---" << std::endl; 

        generation_maze_data.assign(ConfigLoader::MAZE_HEIGHT, std::vector<MazeGeneration::GenCell>(ConfigLoader::MAZE_WIDTH)); 
        solver_maze_data.assign(ConfigLoader::MAZE_HEIGHT, std::vector<MazeSolver::Cell>(ConfigLoader::MAZE_WIDTH)); 
        
        std::cout << "--- Maze Generation (" << algo_info.name << ") ---" << std::endl; 
        int gen_start_r = (ConfigLoader::START_NODE.first >= 0 && ConfigLoader::START_NODE.first < ConfigLoader::MAZE_HEIGHT) ? ConfigLoader::START_NODE.first : 0; 
        int gen_start_c = (ConfigLoader::START_NODE.second >= 0 && ConfigLoader::START_NODE.second < ConfigLoader::MAZE_WIDTH) ? ConfigLoader::START_NODE.second : 0; 

        if (!(ConfigLoader::START_NODE.first >= 0 && ConfigLoader::START_NODE.first < ConfigLoader::MAZE_HEIGHT &&
              ConfigLoader::START_NODE.second >= 0 && ConfigLoader::START_NODE.second < ConfigLoader::MAZE_WIDTH)) { 
             if (algo_info.type == MazeGeneration::MazeAlgorithmType::DFS || algo_info.type == MazeGeneration::MazeAlgorithmType::PRIMS) { 
                std::cout << "Adjusted maze generation start point to (" << gen_start_r << "," << gen_start_c
                          << ") due to out-of-bounds config START_NODE for DFS/Prims." << std::endl; 
             }
        }

        auto start_time_generation = std::chrono::high_resolution_clock::now(); 
        MazeGeneration::generate_maze_structure(generation_maze_data, gen_start_r, gen_start_c, ConfigLoader::MAZE_WIDTH, ConfigLoader::MAZE_HEIGHT, algo_info.type); 
        auto end_time_generation = std::chrono::high_resolution_clock::now(); 
        std::chrono::duration<double> time_taken_generation = end_time_generation - start_time_generation; // Duration in seconds
        std::cout << GREEN_COLOR << std::fixed << std::setprecision(3) << "Time for maze generation: " << time_taken_generation.count() << " s" << RESET_COLOR << std::endl; 

        // Transfer wall data from generation_maze_data to solver_maze_data
        for (int r = 0; r < ConfigLoader::MAZE_HEIGHT; ++r) { 
            for (int c = 0; c < ConfigLoader::MAZE_WIDTH; ++c) { 
                for (int i = 0; i < 4; ++i) { 
                    solver_maze_data[r][c].walls[i] = generation_maze_data[r][c].walls[i]; 
                }
            }
        }
        std::cout << "Maze generated and data transferred." << std::endl; 

        std::cout << "--- BFS Solving & Image Generation (" << algo_info.name << ") ---" << std::endl; 
        auto start_time_bfs = std::chrono::high_resolution_clock::now(); 
        MazeSolver::solve_bfs(solver_maze_data, algo_info.name); 
        auto end_time_bfs = std::chrono::high_resolution_clock::now(); 
        std::chrono::duration<double> time_taken_bfs = end_time_bfs - start_time_bfs; // Duration in seconds
        std::cout << GREEN_COLOR << std::fixed << std::setprecision(3) << "Time for BFS solving & image generation: " << time_taken_bfs.count() << " s" << RESET_COLOR << std::endl; 

        std::cout << "--- DFS Solving & Image Generation (" << algo_info.name << ") ---" << std::endl; 
        auto start_time_dfs = std::chrono::high_resolution_clock::now(); 
        MazeSolver::solve_dfs(solver_maze_data, algo_info.name); 
        auto end_time_dfs = std::chrono::high_resolution_clock::now(); 
        std::chrono::duration<double> time_taken_dfs = end_time_dfs - start_time_dfs; // Duration in seconds
        std::cout << GREEN_COLOR << std::fixed << std::setprecision(3) << "Time for DFS solving & image generation: " << time_taken_dfs.count() << " s" << RESET_COLOR << std::endl; 
    }

    std::cout << "\n--- Processing Complete ---" << std::endl; 
    std::cout << "All selected algorithms processed. Images saved in respective folders." << std::endl; 

    return 0;
}