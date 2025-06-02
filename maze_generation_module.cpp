#include "maze_generation_module.h" // Include the header
#include <algorithm>
#include <random>
#include <vector> // Ensure vector is included for implementation details

// Anonymous namespace for internal linkage (helper functions or variables not exposed)
namespace {
    // Helper recursive function for maze generation
    void generate_maze_recursive_internal(int r, int c,
                                       std::vector<std::vector<MazeGeneration::GenCell>>& current_maze_data,
                                       int M_WIDTH, int M_HEIGHT) {
        current_maze_data[r][c].visited_gen = true;
        std::vector<std::pair<int, int>> directions = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}}; // R, D, L, U
        
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(directions.begin(), directions.end(), g);

        for (const auto& dir : directions) {
            int nr = r + dir.first;
            int nc = c + dir.second;

            if (nr >= 0 && nr < M_HEIGHT && nc >= 0 && nc < M_WIDTH && !current_maze_data[nr][nc].visited_gen) {
                // Carve path
                if (dir.first == 0 && dir.second == 1) { // Right
                    current_maze_data[r][c].walls[1] = false;
                    current_maze_data[nr][nc].walls[3] = false;
                } else if (dir.first == 1 && dir.second == 0) { // Down
                    current_maze_data[r][c].walls[2] = false;
                    current_maze_data[nr][nc].walls[0] = false;
                } else if (dir.first == 0 && dir.second == -1) { // Left
                    current_maze_data[r][c].walls[3] = false;
                    current_maze_data[nr][nc].walls[1] = false;
                } else if (dir.first == -1 && dir.second == 0) { // Up
                    current_maze_data[r][c].walls[0] = false;
                    current_maze_data[nr][nc].walls[2] = false;
                }
                generate_maze_recursive_internal(nr, nc, current_maze_data, M_WIDTH, M_HEIGHT);
            }
        }
    }
} // anonymous namespace


namespace MazeGeneration {

void generate_maze_structure(std::vector<std::vector<GenCell>>& maze_grid_to_populate, 
                             int start_r, int start_c,
                             int grid_width, int grid_height) {
    // Ensure the grid is sized correctly before starting.
    // This could also be a precondition handled by the caller.
    if (maze_grid_to_populate.size() != (size_t)grid_height || 
        (grid_height > 0 && maze_grid_to_populate[0].size() != (size_t)grid_width)) {
        // Resize or throw an error. For now, let's assume it's pre-sized.
        // Or, maze_grid_to_populate.assign(grid_height, std::vector<GenCell>(grid_width));
        // but the caller (main program) will likely manage the 'maze' vector's lifecycle.
    }

    // Validate start_r, start_c against grid_width, grid_height
    if (start_r < 0 || start_r >= grid_height || start_c < 0 || start_c >= grid_width) {
        // Handle error: start coordinates out of bounds.
        // For now, let's assume valid inputs or default to 0,0 if an error occurs
        // This should ideally be reported back or handled more robustly.
        start_r = 0; // Fallback, or throw exception
        start_c = 0; // Fallback
    }

    generate_maze_recursive_internal(start_r, start_c, maze_grid_to_populate, grid_width, grid_height);
}

} // namespace MazeGeneration