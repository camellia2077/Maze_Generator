#ifndef MAZE_GENERATION_MODULE_H
#define MAZE_GENERATION_MODULE_H

#include <vector>
#include <string> // Included for completeness, though not strictly used in this header's interface

// Forward declaration if Cell is more complex and defined elsewhere,
// or define it here if it's simple enough and primarily for generation.
// For this example, let's define a Cell structure suitable for generation.
namespace MazeGeneration {

struct GenCell {
    bool visited_gen = false;
    // Walls: Top, Right, Bottom, Left
    bool walls[4] = {true, true, true, true};
    // Note: No parent here, as that's solver-specific.
    // If the main program's Cell needs to be used, this might need adjustment
    // or the main program's Cell definition would be used directly.
};

// Function to generate the maze data
// It will populate the provided 'maze_grid'
// MAZE_WIDTH and MAZE_HEIGHT are passed to know the grid dimensions.
// The 'maze_grid' in the main program should be passed by reference.
void generate_maze_structure(std::vector<std::vector<GenCell>>& maze_grid_to_populate, 
                             int start_r, int start_c, 
                             int grid_width, int grid_height);

} // namespace MazeGeneration

#endif // MAZE_GENERATION_MODULE_H