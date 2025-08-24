#ifndef MAZE_SOLVER_H
#define MAZE_SOLVER_H

#include <vector>
#include <string>
#include <utility> // For std::pair
#include <queue>   // For BFS queue
#include <stack>   // For DFS stack

// Forward declare if ConfigLoader types are minimally used in signatures,
// but since save_image (even if internal) uses many ConfigLoader constants,
// its implementation in maze_solver.cpp will include "config_loader.h".

namespace MazeSolver {

    // Maze Cell Structure for the solver
    // Contains wall information and parent tracking for path reconstruction.
    struct Cell {
        bool walls[4] = {true, true, true, true}; // Top, Right, Bottom, Left
        std::pair<int, int> parent = {-1, -1};    // Stores the coordinate of the parent cell in the path
    };

    // Enum to represent the visual state of a cell during solving for image generation.
    enum class SolverCellState {
        NONE,           // Default state
        START,          // Start node
        END,            // End node
        FRONTIER,       // Cell is in the queue/stack to be visited
        CURRENT_PROC,   // Cell is currently being processed
        VISITED_PROC,   // Cell has been processed
        SOLUTION        // Cell is part of the final solution path
    };

    // Helper function to determine if three points form a straight line.
    // Used to optimize frame saving by skipping frames for straight path segments.
    bool is_straight_line(std::pair<int, int> p1, std::pair<int, int> p2, std::pair<int, int> p3);

    // Declaration for the image saving function.
    // This is used internally by the solvers to generate visualization frames.
    void save_image(const std::string& folder, int step_count,
                    const std::vector<std::vector<SolverCellState>>& visual_states,
                    const std::vector<std::vector<Cell>>& maze_ref, // Pass maze data by const reference
                    const std::vector<std::pair<int, int>>& current_path = {});

    // Declaration for the Breadth-First Search (BFS) solver.
    // Modifies maze_data to store parent information for path reconstruction.
    void solve_bfs(std::vector<std::vector<Cell>>& maze_data,
                   const std::string& generation_algorithm_name);

    // Declaration for the Depth-First Search (DFS) solver.
    // Modifies maze_data to store parent information for path reconstruction.
    void solve_dfs(std::vector<std::vector<Cell>>& maze_data,
                   const std::string& generation_algorithm_name);

} // namespace MazeSolver

#endif // MAZE_SOLVER_H