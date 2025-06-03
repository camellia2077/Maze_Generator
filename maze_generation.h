#ifndef maze_generation_H
#define maze_generation_H

#include <vector>
#include <string> // Included for completeness, though not strictly used in this header's interface

namespace MazeGeneration {

// Enum to specify the maze generation algorithm
enum class MazeAlgorithmType {
    DFS,    // Recursive Backtracker (Randomized DFS)
    PRIMS,  // Randomized Prim's Algorithm
    KRUSKAL // Randomized Kruskal's Algorithm
};

struct GenCell {
    bool visited_gen = false; // 单元格在生成过程中是否被访问
    // Walls: Top, Right, Bottom, Left
    bool walls[4] = {true, true, true, true};
};

// Function to generate the maze data
// It will populate the provided 'maze_grid'
// grid_width and grid_height are passed to know the grid dimensions.
// The 'maze_grid' in the main program should be passed by reference.
void generate_maze_structure(std::vector<std::vector<GenCell>>& maze_grid_to_populate,
                             int start_r, int start_c, // row and col are less relevant for Kruskal's
                             int grid_width, int grid_height,
                             MazeAlgorithmType algorithm_type);

} // namespace MazeGeneration

#endif // maze_generation_H