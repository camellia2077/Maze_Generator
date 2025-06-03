#include "maze_generation.h" // Include the header
#include <algorithm>
#include <random>
#include <vector>
#include <iostream> // For error messages
#include <numeric>

// Anonymous namespace for internal linkage (helper functions or variables not exposed)
namespace {

    // Helper recursive function for DFS (Recursive Backtracker) maze generation
    //深度优先搜索 (DFS / Recursive Backtracker) 
    void generate_maze_recursive_internal(int r, int c,//row and col
                                       std::vector<std::vector<MazeGeneration::GenCell>>& current_maze_data,
                                       int M_WIDTH, int M_HEIGHT) 
    {
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

    // Helper structure for Prim's Algorithm
    struct FrontierEdge {
        int r1, c1; // Visited cell
        int r2, c2; // Unvisited cell
        int wall_idx_r1; // Wall index in (r1, c1) to carve (0:T, 1:R, 2:B, 3:L)
        int wall_idx_r2; // Wall index in (r2, c2) to carve (0:T, 1:R, 2:B, 3:L)
    };

    // Helper function for Prim's Algorithm maze generation普里姆算法 (Prim's Algorithm)
    void generate_maze_prims_internal(int start_r_prim, int start_c_prim,
                                      std::vector<std::vector<MazeGeneration::GenCell>>& current_maze_data,
                                      int M_WIDTH, int M_HEIGHT) {
        // Ensure all cells are marked unvisited and walls are up initially for this generation pass
        for (int r_init = 0; r_init < M_HEIGHT; ++r_init) {
            for (int c_init = 0; c_init < M_WIDTH; ++c_init) {
                current_maze_data[r_init][c_init].visited_gen = false;
                for(int i=0; i<4; ++i) current_maze_data[r_init][c_init].walls[i] = true;
            }
        }

        std::random_device rd;
        std::mt19937 g(rd());

        int r = start_r_prim;
        int c = start_c_prim;
        current_maze_data[r][c].visited_gen = true;

        std::vector<FrontierEdge> frontier_walls;

        // Directions (dr, dc): Up, Right, Down, Left
        int dr[] = {-1, 0, 1, 0}; 
        int dc[] = {0, 1, 0, -1};
        // Wall indices for current cell (relative to cell's perspective): Top, Right, Bottom, Left
        int wall_indices[] = {0, 1, 2, 3}; 
        // Opposite wall indices for the neighbor cell: Bottom, Left, Top, Right
        int opposite_wall_indices[] = {2, 3, 0, 1}; 

        // Add walls of the starting cell to the frontier list
        for (int i = 0; i < 4; ++i) {
            int nr = r + dr[i];
            int nc = c + dc[i];
            if (nr >= 0 && nr < M_HEIGHT && nc >= 0 && nc < M_WIDTH) {
                // At this point, (nr, nc) is unvisited as only (r,c) is visited
                frontier_walls.push_back({r, c, nr, nc, wall_indices[i], opposite_wall_indices[i]});
            }
        }

        while (!frontier_walls.empty()) {
            std::uniform_int_distribution<> distrib(0, frontier_walls.size() - 1);
            int rand_idx = distrib(g);
            FrontierEdge edge = frontier_walls[rand_idx];
            
            frontier_walls[rand_idx] = frontier_walls.back(); // Efficient removal
            frontier_walls.pop_back();

            // (edge.r1, edge.c1) is the visited cell.
            // (edge.r2, edge.c2) is the potentially unvisited cell.
            if (!current_maze_data[edge.r2][edge.c2].visited_gen) {
                // Carve the wall
                current_maze_data[edge.r1][edge.c1].walls[edge.wall_idx_r1] = false;
                current_maze_data[edge.r2][edge.c2].walls[edge.wall_idx_r2] = false;

                // Mark the new cell (edge.r2, edge.c2) as visited
                current_maze_data[edge.r2][edge.c2].visited_gen = true;

                // Add new frontier walls from the newly visited cell (edge.r2, edge.c2)
                for (int i = 0; i < 4; ++i) {
                    int nnr = edge.r2 + dr[i]; 
                    int nnc = edge.c2 + dc[i]; 

                    if (nnr >= 0 && nnr < M_HEIGHT && nnc >= 0 && nnc < M_WIDTH &&
                        !current_maze_data[nnr][nnc].visited_gen) {
                        frontier_walls.push_back({edge.r2, edge.c2, nnr, nnc, wall_indices[i], opposite_wall_indices[i]});
                    }
                }
            }
        }
    }

    // --- Kruskal's Algorithm Helpers ---
    struct WallEdge {
        int r1, c1; // Cell 1
        int r2, c2; // Cell 2
        int wall_idx_c1; // Wall index in (r1, c1) to carve (0:T, 1:R, 2:B, 3:L)
        int wall_idx_c2; // Wall index in (r2, c2) to carve (0:T, 1:R, 2:B, 3:L)
    };

    class DSU {
    public:
        DSU(int n) : parent(n), rank(n, 0) {
            std::iota(parent.begin(), parent.end(), 0); // Fill with 0, 1, 2, ...
        }

        int find(int i) {
            if (parent[i] == i)
                return i;
            return parent[i] = find(parent[i]); // Path compression
        }

        void unite(int i, int j) {
            int root_i = find(i);
            int root_j = find(j);
            if (root_i != root_j) {
                if (rank[root_i] < rank[root_j]) std::swap(root_i, root_j);
                parent[root_j] = root_i;
                if (rank[root_i] == rank[root_j]) rank[root_i]++;
            }
        }
    private:
        std::vector<int> parent;
        std::vector<int> rank;
    };
    //克鲁斯卡尔算法 (Kruskal's Algorithm) 
    void generate_maze_kruskal_internal(std::vector<std::vector<MazeGeneration::GenCell>>& current_maze_data,
                                        int M_WIDTH, int M_HEIGHT) {
        // Ensure all walls are up initially for this generation pass
        for (int r_init = 0; r_init < M_HEIGHT; ++r_init) {
            for (int c_init = 0; c_init < M_WIDTH; ++c_init) {
                 current_maze_data[r_init][c_init].visited_gen = false; // Reset for good measure
                for(int i=0; i<4; ++i) current_maze_data[r_init][c_init].walls[i] = true;
            }
        }

        std::vector<WallEdge> all_walls;
        // Populate all_walls list
        // Wall indices reminder: 0:Top, 1:Right, 2:Bottom, 3:Left
        for (int r = 0; r < M_HEIGHT; ++r) {
            for (int c = 0; c < M_WIDTH; ++c) {
                if (c < M_WIDTH - 1) { // Horizontal wall (Right of current cell)
                    all_walls.push_back({r, c, r, c + 1, 1, 3});
                }
                if (r < M_HEIGHT - 1) { // Vertical wall (Bottom of current cell)
                    all_walls.push_back({r, c, r + 1, c, 2, 0});
                }
            }
        }

        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(all_walls.begin(), all_walls.end(), g);

        DSU dsu(M_WIDTH * M_HEIGHT);
        int num_edges_added = 0;
        int total_cells = M_WIDTH * M_HEIGHT;

        for (const auto& wall_edge : all_walls) {
            if (num_edges_added >= total_cells -1 && total_cells > 0) break; // Optimization: stop if all cells are connected

            int cell1_idx = wall_edge.r1 * M_WIDTH + wall_edge.c1;
            int cell2_idx = wall_edge.r2 * M_WIDTH + wall_edge.c2;

            if (dsu.find(cell1_idx) != dsu.find(cell2_idx)) {
                // Remove the wall
                current_maze_data[wall_edge.r1][wall_edge.c1].walls[wall_edge.wall_idx_c1] = false;
                current_maze_data[wall_edge.r2][wall_edge.c2].walls[wall_edge.wall_idx_c2] = false;
                dsu.unite(cell1_idx, cell2_idx);
                num_edges_added++;
            }
        }
    }


} // anonymous namespace


namespace MazeGeneration {

void generate_maze_structure(std::vector<std::vector<GenCell>>& maze_grid_to_populate, 
                             int start_r, int start_c, // start_r, start_c are hints/ignored by some algos
                             int grid_width, int grid_height,
                             MazeAlgorithmType algorithm_type) { 

    // Ensure the grid is sized correctly (caller's responsibility, but check start coords for algos that use it)
    if (algorithm_type == MazeAlgorithmType::DFS || algorithm_type == MazeAlgorithmType::PRIMS) { // Kruskal's doesn't use start_r/c
        if (start_r < 0 || start_r >= grid_height || start_c < 0 || start_c >= grid_width) {
            std::cerr << "Warning: Maze generation start coordinates (" << start_r << "," << start_c 
                      << ") out of bounds for grid (" << grid_height << "x" << grid_width 
                      << ") for DFS/Prims. Defaulting to (0,0)." << std::endl;
            start_r = 0; 
            start_c = 0;
        }
    }


    // Ensure all cells are in a known initial state (walls up, unvisited).
    // This is also done within Prim's and Kruskal's internal functions, but good for robustness.
    for (int r_init = 0; r_init < grid_height; ++r_init) {
        for (int c_init = 0; c_init < grid_width; ++c_init) {
            maze_grid_to_populate[r_init][c_init].visited_gen = false;
            for(int i=0; i<4; ++i) maze_grid_to_populate[r_init][c_init].walls[i] = true;
        }
    }


    switch (algorithm_type) {
        case MazeAlgorithmType::DFS:
            std::cout << "Using DFS (Recursive Backtracker) for maze generation." << std::endl;
            generate_maze_recursive_internal(start_r, start_c, maze_grid_to_populate, grid_width, grid_height);
            break;
        case MazeAlgorithmType::PRIMS:
            std::cout << "Using Prim's Algorithm for maze generation." << std::endl;
            generate_maze_prims_internal(start_r, start_c, maze_grid_to_populate, grid_width, grid_height);
            break;
        case MazeAlgorithmType::KRUSKAL:
            std::cout << "Using Kruskal's Algorithm for maze generation." << std::endl;
            generate_maze_kruskal_internal(maze_grid_to_populate, grid_width, grid_height);
            break;
        default:
            std::cerr << "Error: Unknown maze generation algorithm specified. Defaulting to DFS." << std::endl;
            generate_maze_recursive_internal(start_r, start_c, maze_grid_to_populate, grid_width, grid_height);
            break;
    }
}

} 