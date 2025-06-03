#include "maze_solver.h"
#include "config_loader.h" // Required for accessing configuration like colors, dimensions

#include <iostream>        // For std::cout, std::cerr
#include <iomanip>         // For std::setw, std::setfill (used in save_image)
#include <filesystem>      // For fs::create_directories
#include <sstream>         // For std::stringstream (used in save_image)
#include <algorithm>       // For std::reverse (used in path reconstruction), std::min

// Define STB_IMAGE_WRITE_IMPLEMENTATION in exactly one .c or .cpp file
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace fs = std::filesystem;

namespace MazeSolver {

// --- Helper function to check if three points form a straight line ---
bool is_straight_line(std::pair<int, int> p1, std::pair<int, int> p2, std::pair<int, int> p3) {
    if (p1.first == -1 || p1.second == -1 ||
        p2.first == -1 || p2.second == -1 ||
        p3.first == -1 || p3.second == -1) {
        return false;
    }
    int dr1 = p2.first - p1.first;
    int dc1 = p2.second - p1.second;
    int dr2 = p3.first - p2.first;
    int dc2 = p3.second - p2.second;
    return dr1 == dr2 && dc1 == dc2;
}

// --- Image Saving Function ---
void save_image(const std::string& folder, int step_count,
                const std::vector<std::vector<SolverCellState>>& visual_states,
                const std::vector<std::vector<Cell>>& maze_ref, // Changed from global 'maze'
                const std::vector<std::pair<int, int>>& current_path) {

    if (ConfigLoader::MAZE_WIDTH <= 0 || ConfigLoader::MAZE_HEIGHT <= 0 || ConfigLoader::UNIT_PIXELS <= 0) {
        std::cerr << "Error: Invalid maze dimensions or unit pixels for saving image. Aborting save." << std::endl;
        return;
    }

    int img_width_units = 2 * ConfigLoader::MAZE_WIDTH + 1;
    int img_height_units = 2 * ConfigLoader::MAZE_HEIGHT + 1;
    int final_img_width = img_width_units * ConfigLoader::UNIT_PIXELS;
    int final_img_height = img_height_units * ConfigLoader::UNIT_PIXELS;

    std::vector<unsigned char> pixels(final_img_width * final_img_height * 3);

    for (int r_unit = 0; r_unit < img_height_units; ++r_unit) {
        for (int c_unit = 0; c_unit < img_width_units; ++c_unit) {
            const unsigned char* current_color_ptr; // Will be assigned below

            // Determine if the current unit is part of the outer frame
            bool is_outer_frame_unit = (r_unit == 0 || r_unit == img_height_units - 1 ||
                                        c_unit == 0 || c_unit == img_width_units - 1);

            if (is_outer_frame_unit) {
                current_color_ptr = ConfigLoader::COLOR_OUTER_WALL;
            } else if (r_unit % 2 != 0 && c_unit % 2 != 0) { // Cell interior (and not outer frame)
                int maze_r = (r_unit - 1) / 2;
                int maze_c = (c_unit - 1) / 2;
                current_color_ptr = ConfigLoader::COLOR_BACKGROUND;

                SolverCellState state = visual_states[maze_r][maze_c];
                if (state == SolverCellState::START) current_color_ptr = ConfigLoader::COLOR_START;
                else if (state == SolverCellState::END) current_color_ptr = ConfigLoader::COLOR_END;
                else if (state == SolverCellState::SOLUTION) current_color_ptr = ConfigLoader::COLOR_SOLUTION_PATH;
                else if (state == SolverCellState::CURRENT_PROC) current_color_ptr = ConfigLoader::COLOR_CURRENT;
                else if (state == SolverCellState::FRONTIER) current_color_ptr = ConfigLoader::COLOR_FRONTIER;
                else if (state == SolverCellState::VISITED_PROC) current_color_ptr = ConfigLoader::COLOR_VISITED;

                if (state != SolverCellState::SOLUTION) {
                     if (maze_r == ConfigLoader::START_NODE.first && maze_c == ConfigLoader::START_NODE.second && state != SolverCellState::END) current_color_ptr = ConfigLoader::COLOR_START;
                     if (maze_r == ConfigLoader::END_NODE.first && maze_c == ConfigLoader::END_NODE.second && state != SolverCellState::START) current_color_ptr = ConfigLoader::COLOR_END;
                }

            } else if (r_unit % 2 != 0 && c_unit % 2 == 0) { // Potential Vertical Wall (and not outer frame)
                int maze_r = (r_unit - 1) / 2;
                int maze_c_left = (c_unit / 2) - 1;
                // Since it's not an outer frame unit, c_unit > 0 and c_unit < 2*MAZE_WIDTH are implied for internal wall columns
                if (maze_r >= 0 && maze_r < ConfigLoader::MAZE_HEIGHT && maze_c_left >= 0 && maze_c_left < ConfigLoader::MAZE_WIDTH) {
                   if (!maze_ref[maze_r][maze_c_left].walls[1]) current_color_ptr = ConfigLoader::COLOR_BACKGROUND; // Check right wall of left cell
                   else current_color_ptr = ConfigLoader::COLOR_INNER_WALL;
                } else { // Should not happen if not outer frame, but as a fallback
                    current_color_ptr = ConfigLoader::COLOR_INNER_WALL;
                }
            } else if (r_unit % 2 == 0 && c_unit % 2 != 0) { // Potential Horizontal Wall (and not outer frame)
                int maze_r_up = (r_unit / 2) - 1;
                int maze_c = (c_unit - 1) / 2;
                // Since it's not an outer frame unit, r_unit > 0 and r_unit < 2*MAZE_HEIGHT are implied
                if (maze_r_up >= 0 && maze_r_up < ConfigLoader::MAZE_HEIGHT && maze_c >=0 && maze_c < ConfigLoader::MAZE_WIDTH) {
                    if (!maze_ref[maze_r_up][maze_c].walls[2]) current_color_ptr = ConfigLoader::COLOR_BACKGROUND; // Check bottom wall of up cell
                    else current_color_ptr = ConfigLoader::COLOR_INNER_WALL;
                } else { // Should not happen
                     current_color_ptr = ConfigLoader::COLOR_INNER_WALL;
                }
            } else { // Corner/Junction points (and not outer frame)
                current_color_ptr = ConfigLoader::COLOR_INNER_WALL;
            }

            for (int py = 0; py < ConfigLoader::UNIT_PIXELS; ++py) {
                for (int px = 0; px < ConfigLoader::UNIT_PIXELS; ++px) {
                    int img_x = c_unit * ConfigLoader::UNIT_PIXELS + px;
                    int img_y = r_unit * ConfigLoader::UNIT_PIXELS + py;
                    if (img_x < final_img_width && img_y < final_img_height) {
                        int idx = (img_y * final_img_width + img_x) * 3;
                        pixels[idx] = current_color_ptr[0];
                        pixels[idx + 1] = current_color_ptr[1];
                        pixels[idx + 2] = current_color_ptr[2];
                    }
                }
            }
        }
    }

    // Draw the explicit path (corridors and cells) if provided - this logic remains the same
    for(const auto& p : current_path) {
        int maze_r = p.first;
        int maze_c = p.second;
        const unsigned char* path_color_to_use_ptr = ConfigLoader::COLOR_SOLUTION_PATH;
        if (p == ConfigLoader::START_NODE) path_color_to_use_ptr = ConfigLoader::COLOR_START;
        else if (p == ConfigLoader::END_NODE) path_color_to_use_ptr = ConfigLoader::COLOR_END;

        for (int py = 0; py < ConfigLoader::UNIT_PIXELS; ++py) {
            for (int px = 0; px < ConfigLoader::UNIT_PIXELS; ++px) {
                int img_x = (2 * maze_c + 1) * ConfigLoader::UNIT_PIXELS + px;
                int img_y = (2 * maze_r + 1) * ConfigLoader::UNIT_PIXELS + py;
                 if (img_x < final_img_width && img_y < final_img_height) {
                    int idx = (img_y * final_img_width + img_x) * 3;
                    pixels[idx] = path_color_to_use_ptr[0];
                    pixels[idx + 1] = path_color_to_use_ptr[1];
                    pixels[idx + 2] = path_color_to_use_ptr[2];
                }
            }
        }
    }
    // Draw corridors for the current_path - this logic remains the same
    if (current_path.size() > 1) {
        for (size_t i = 0; i < current_path.size() - 1; ++i) {
            std::pair<int, int> p1 = current_path[i];
            std::pair<int, int> p2 = current_path[i+1];

            int r1 = p1.first; int c1 = p1.second;
            int r2 = p2.first; int c2 = p2.second;

            int corridor_r_unit = -1, corridor_c_unit = -1;
            const unsigned char* corridor_color_ptr = ConfigLoader::COLOR_SOLUTION_PATH;

            if (r1 == r2) { // Horizontal corridor
                corridor_r_unit = 2 * r1 + 1;
                corridor_c_unit = 2 * std::min(c1, c2) + 2;
            } else if (c1 == c2) { // Vertical corridor
                corridor_c_unit = 2 * c1 + 1;
                corridor_r_unit = 2 * std::min(r1, r2) + 2;
            }

            if (corridor_r_unit != -1 && corridor_c_unit != -1) {
                for (int py = 0; py < ConfigLoader::UNIT_PIXELS; ++py) {
                    for (int px = 0; px < ConfigLoader::UNIT_PIXELS; ++px) {
                        int img_x = corridor_c_unit * ConfigLoader::UNIT_PIXELS + px;
                        int img_y = corridor_r_unit * ConfigLoader::UNIT_PIXELS + py;
                        if (img_x < final_img_width && img_y < final_img_height) {
                            int idx = (img_y * final_img_width + img_x) * 3;
                            pixels[idx]     = corridor_color_ptr[0];
                            pixels[idx + 1] = corridor_color_ptr[1];
                            pixels[idx + 2] = corridor_color_ptr[2];
                        }
                    }
                }
            }
        }
    }

    std::stringstream ss;
    ss << folder << "/frame_" << std::setw(4) << std::setfill('0') << step_count << ".png";
    stbi_write_png(ss.str().c_str(), final_img_width, final_img_height, 3, pixels.data(), final_img_width * 3);
}


// --- BFS Solver ---
// ... (solve_bfs function remains unchanged from the provided file) ...
void solve_bfs(std::vector<std::vector<Cell>>& maze_data, // Use passed maze_data
               const std::string& generation_algorithm_name) {
    std::string folder = "bfs_frames_generated_by_" + generation_algorithm_name;
    fs::create_directories(folder);
    std::cout << "Starting BFS for maze generated by " << generation_algorithm_name << " (frames in " << folder << ")..." << std::endl;

    if (ConfigLoader::MAZE_WIDTH <=0 || ConfigLoader::MAZE_HEIGHT <=0) {
        std::cout << "BFS: Invalid maze dimensions. Aborting." << std::endl;
        return;
    }
     if (ConfigLoader::START_NODE == ConfigLoader::END_NODE) {
        std::cout << "BFS: Start and End nodes are the same. Path is trivial." << std::endl;
        std::vector<std::vector<SolverCellState>> visual_states(ConfigLoader::MAZE_HEIGHT, std::vector<SolverCellState>(ConfigLoader::MAZE_WIDTH, SolverCellState::NONE));
        if(ConfigLoader::START_NODE.first < ConfigLoader::MAZE_HEIGHT && ConfigLoader::START_NODE.second < ConfigLoader::MAZE_WIDTH) {
            visual_states[ConfigLoader::START_NODE.first][ConfigLoader::START_NODE.second] = SolverCellState::SOLUTION;
        }
        save_image(folder, 0, visual_states, maze_data, {{ConfigLoader::START_NODE}});
        return;
    }

    std::vector<std::vector<SolverCellState>> visual_states(ConfigLoader::MAZE_HEIGHT, std::vector<SolverCellState>(ConfigLoader::MAZE_WIDTH, SolverCellState::NONE));
    std::vector<std::vector<bool>> visited_solver(ConfigLoader::MAZE_HEIGHT, std::vector<bool>(ConfigLoader::MAZE_WIDTH, false));
    std::queue<std::pair<int, int>> q;

    for(int r=0; r<ConfigLoader::MAZE_HEIGHT; ++r) {
        for(int c=0; c<ConfigLoader::MAZE_WIDTH; ++c) {
            maze_data[r][c].parent = {-1, -1}; // Initialize parent for maze_data
        }
    }

    q.push(ConfigLoader::START_NODE);
    if (ConfigLoader::START_NODE.first < ConfigLoader::MAZE_HEIGHT && ConfigLoader::START_NODE.second < ConfigLoader::MAZE_WIDTH) {
        visited_solver[ConfigLoader::START_NODE.first][ConfigLoader::START_NODE.second] = true;
        visual_states[ConfigLoader::START_NODE.first][ConfigLoader::START_NODE.second] = SolverCellState::FRONTIER;
    } else {
        std::cerr << "BFS: Start node is out of bounds. Aborting." << std::endl;
        return;
    }

    int step = 0;
    save_image(folder, step++, visual_states, maze_data);

    std::vector<std::pair<int, int>> path;
    bool found = false;

    // Directions: Up, Down, Left, Right (for dr, dc)
    // Corresponding wall checks: Top (0), Bottom (2), Left (3), Right (1)
    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};
    // Wall indices for current cell: maze_data[curr.first][curr.second].walls[wall_check_indices[i]]
    // If moving Up (dr=-1), check current cell's Top wall (walls[0])
    // If moving Down (dr=1), check current cell's Bottom wall (walls[2])
    // If moving Left (dc=-1), check current cell's Left wall (walls[3])
    // If moving Right (dc=1), check current cell's Right wall (walls[1])
    int wall_check_indices[] = {0, 2, 3, 1};

    while (!q.empty() && !found) {
        std::pair<int, int> curr = q.front();
        q.pop();

        bool should_save_current_step_frames = true;
        std::pair<int, int> parent_cell = maze_data[curr.first][curr.second].parent;
        if (parent_cell.first != -1 && parent_cell.second != -1) {
            std::pair<int, int> grandparent_cell = maze_data[parent_cell.first][parent_cell.second].parent;
            if (grandparent_cell.first != -1 && grandparent_cell.second != -1) {
                if (is_straight_line(grandparent_cell, parent_cell, curr)) {
                    should_save_current_step_frames = false;
                }
            }
        }
        if (curr == ConfigLoader::END_NODE) {
            should_save_current_step_frames = true; // Always save frame for reaching the end
        }
        
        visual_states[curr.first][curr.second] = SolverCellState::CURRENT_PROC;
        if (should_save_current_step_frames) {
            save_image(folder, step++, visual_states, maze_data);
        }


        if (curr == ConfigLoader::END_NODE) {
            found = true;
        }

        if (!found) {
            for (int i = 0; i < 4; ++i) {
                int nr = curr.first + dr[i];
                int nc = curr.second + dc[i];

                // Check if the wall exists from the perspective of the current cell
                bool wall_exists = maze_data[curr.first][curr.second].walls[wall_check_indices[i]];

                if (nr >= 0 && nr < ConfigLoader::MAZE_HEIGHT && nc >= 0 && nc < ConfigLoader::MAZE_WIDTH &&
                    !visited_solver[nr][nc] && !wall_exists) {

                    visited_solver[nr][nc] = true;
                    maze_data[nr][nc].parent = curr; // Use maze_data
                    q.push({nr, nc});
                    visual_states[nr][nc] = SolverCellState::FRONTIER;
                }
            }
        }
        
        if (curr != ConfigLoader::END_NODE || !found) { // If not end or not found, mark as visited
             visual_states[curr.first][curr.second] = SolverCellState::VISITED_PROC;
        }


        if (should_save_current_step_frames) { // Save after marking visited or if it was current_proc and saved
             save_image(folder, step++, visual_states, maze_data);
        }
        if (found && curr == ConfigLoader::END_NODE) break; // Exit if path found and current is end node
    }

    if (found) {
        std::pair<int, int> curr_path_node = ConfigLoader::END_NODE;
        while (curr_path_node.first != -1 && curr_path_node.second != -1) {
            path.push_back(curr_path_node);
            visual_states[curr_path_node.first][curr_path_node.second] = SolverCellState::SOLUTION;
             if (curr_path_node == ConfigLoader::START_NODE) break; // Stop if start node is reached
            curr_path_node = maze_data[curr_path_node.first][curr_path_node.second].parent; // Use maze_data
        }
        std::reverse(path.begin(), path.end());
        save_image(folder, step++, visual_states, maze_data, path); // Pass maze_data and path
        std::cout << "BFS: Path found. Length: " << path.size() << std::endl;
    } else {
        std::cout << "BFS: Path not found." << std::endl;
        save_image(folder, step++, visual_states, maze_data); // Pass maze_data
    }
}

// --- DFS Solver ---
// ... (solve_dfs function remains unchanged from the provided file) ...
void solve_dfs(std::vector<std::vector<Cell>>& maze_data, // Use passed maze_data
               const std::string& generation_algorithm_name) {
    std::string folder = "dfs_frames_generated_by_" + generation_algorithm_name;
    fs::create_directories(folder);
    std::cout << "Starting DFS for maze generated by " << generation_algorithm_name << " (frames in " << folder << ")..." << std::endl;

    if (ConfigLoader::MAZE_WIDTH <=0 || ConfigLoader::MAZE_HEIGHT <=0) {
        std::cout << "DFS: Invalid maze dimensions. Aborting." << std::endl;
        return;
    }
    if (ConfigLoader::START_NODE == ConfigLoader::END_NODE) {
        std::cout << "DFS: Start and End nodes are the same. Path is trivial." << std::endl;
        std::vector<std::vector<SolverCellState>> visual_states(ConfigLoader::MAZE_HEIGHT, std::vector<SolverCellState>(ConfigLoader::MAZE_WIDTH, SolverCellState::NONE));
        if(ConfigLoader::START_NODE.first < ConfigLoader::MAZE_HEIGHT && ConfigLoader::START_NODE.second < ConfigLoader::MAZE_WIDTH){
            visual_states[ConfigLoader::START_NODE.first][ConfigLoader::START_NODE.second] = SolverCellState::SOLUTION;
        }
        save_image(folder, 0, visual_states, maze_data, {{ConfigLoader::START_NODE}});
        return;
    }

    std::vector<std::vector<SolverCellState>> visual_states(ConfigLoader::MAZE_HEIGHT, std::vector<SolverCellState>(ConfigLoader::MAZE_WIDTH, SolverCellState::NONE));
    std::vector<std::vector<bool>> visited_solver(ConfigLoader::MAZE_HEIGHT, std::vector<bool>(ConfigLoader::MAZE_WIDTH, false));
    std::stack<std::pair<int, int>> s;

    for(int r=0; r<ConfigLoader::MAZE_HEIGHT; ++r) {
        for(int c=0; c<ConfigLoader::MAZE_WIDTH; ++c) {
            maze_data[r][c].parent = {-1, -1}; // Initialize parent for maze_data
        }
    }
    
    s.push(ConfigLoader::START_NODE);
    if (ConfigLoader::START_NODE.first < ConfigLoader::MAZE_HEIGHT && ConfigLoader::START_NODE.second < ConfigLoader::MAZE_WIDTH) {
        // Don't mark visited here, DFS marks visited when processing from stack
        visual_states[ConfigLoader::START_NODE.first][ConfigLoader::START_NODE.second] = SolverCellState::FRONTIER;
    } else {
         std::cerr << "DFS: Start node is out of bounds. Aborting." << std::endl;
        return;
    }
    int step = 0;
    save_image(folder, step++, visual_states, maze_data); // Pass maze_data

    std::vector<std::pair<int, int>> path;
    bool found = false;

    // Directions: Up, Right, Down, Left (typical DFS exploration order)
    int dr[] = {-1, 0, 1, 0};
    int dc[] = {0, 1, 0, -1};
    // Corresponding wall checks for current cell: walls[0] (Top), walls[1] (Right), walls[2] (Bottom), walls[3] (Left)
    int wall_check_indices[] = {0, 1, 2, 3};

    while (!s.empty() && !found) {
        std::pair<int, int> curr = s.top();
        // Don't pop yet, pop only if no unvisited neighbors or if already processed

        bool should_save_frame_for_curr = true; // Assume we save unless it's a straight line skip

        // If current cell was marked as VISITED_PROC from a previous pop (backtrack),
        // it means all its paths were explored. So, pop it and continue.
        if(visual_states[curr.first][curr.second] == SolverCellState::VISITED_PROC) {
             s.pop();
             // No frame saving here, as this state was already visualized during backtracking.
             continue; 
        }


        if (!visited_solver[curr.first][curr.second]) {
             visited_solver[curr.first][curr.second] = true;
             visual_states[curr.first][curr.second] = SolverCellState::CURRENT_PROC;

            // Straight line optimization check
            std::pair<int, int> parent_cell = maze_data[curr.first][curr.second].parent;
            if (parent_cell.first != -1 && parent_cell.second != -1) { // Check if parent exists
                std::pair<int, int> grandparent_cell = maze_data[parent_cell.first][parent_cell.second].parent;
                if (grandparent_cell.first != -1 && grandparent_cell.second != -1) { // Check if grandparent exists
                    if (is_straight_line(grandparent_cell, parent_cell, curr)) {
                        should_save_frame_for_curr = false;
                    }
                }
            }
            if (curr == ConfigLoader::END_NODE) should_save_frame_for_curr = true; // Always save if it's the end node

            if (should_save_frame_for_curr) {
                save_image(folder, step++, visual_states, maze_data); // Pass maze_data
            }
        }


        if (curr == ConfigLoader::END_NODE) {
            found = true;
            // Don't break yet, let the path reconstruction happen after loop
            // The final solution frame will be saved outside.
            // The current frame for END_NODE being CURRENT_PROC is already saved.
        }

        if (found) break; // If found, exit to reconstruct path

        bool pushed_neighbor = false;
        // Explore neighbors
        for (int i = 0; i < 4; ++i) { // Iterate through neighbors
            int nr = curr.first + dr[i];
            int nc = curr.second + dc[i];

            bool wall_exists = maze_data[curr.first][curr.second].walls[wall_check_indices[i]];

            if (nr >= 0 && nr < ConfigLoader::MAZE_HEIGHT && nc >= 0 && nc < ConfigLoader::MAZE_WIDTH &&
                !visited_solver[nr][nc] && !wall_exists) {
                
                maze_data[nr][nc].parent = curr; // Use maze_data
                s.push({nr, nc});
                visual_states[nr][nc] = SolverCellState::FRONTIER; // Mark new neighbor as frontier
                pushed_neighbor = true;
                // Optional: save frame showing new frontier cells
                // if (should_save_frame_for_curr) { // Or a separate condition for frontier updates
                //    save_image(folder, step++, visual_states, maze_data);
                // }
                // For DFS, typically we only show the main path exploration, so break to go deep
                break; 
            }
        }

        if (!pushed_neighbor) { // If no unvisited neighbor was pushed, this path is a dead end or all neighbors visited
            s.pop(); // Backtrack
            visual_states[curr.first][curr.second] = SolverCellState::VISITED_PROC; // Mark as fully explored/backtracked

            // Save frame for backtracking step, unless it was a straight line skip previously AND not the start/end node.
            bool re_save_on_backtrack = true;
            std::pair<int, int> parent_cell = maze_data[curr.first][curr.second].parent;
            if(parent_cell.first != -1 && parent_cell.second != -1) {
                std::pair<int, int> grandparent_cell = maze_data[parent_cell.first][parent_cell.second].parent;
                if(grandparent_cell.first != -1 && grandparent_cell.second != -1 && 
                   is_straight_line(grandparent_cell, parent_cell, curr) && 
                   curr != ConfigLoader::END_NODE && curr != ConfigLoader::START_NODE) { // Don't skip saving backtrack from start/end
                    re_save_on_backtrack = false;
                }
            }
            if (curr == ConfigLoader::START_NODE && !pushed_neighbor) re_save_on_backtrack = true; // Always show final state of start if no path
            
            if(re_save_on_backtrack){
                 save_image(folder, step++, visual_states, maze_data); // Pass maze_data
            }
        }
        // If a neighbor was pushed, the loop continues with the new 'curr' from stack top in next iteration
    }

    if (found) {
        std::pair<int, int> curr_path_node = ConfigLoader::END_NODE;
        while (curr_path_node.first != -1 && curr_path_node.second != -1) {
            path.push_back(curr_path_node);
             // Ensure visual state is SOLUTION for path drawing
            visual_states[curr_path_node.first][curr_path_node.second] = SolverCellState::SOLUTION;
            if (curr_path_node == ConfigLoader::START_NODE) break;
            curr_path_node = maze_data[curr_path_node.first][curr_path_node.second].parent; // Use maze_data
        }
        std::reverse(path.begin(), path.end());
        // Final image with the complete solution path highlighted
        save_image(folder, step++, visual_states, maze_data, path); // Pass maze_data and path
        std::cout << "DFS: Path found. Length: " << path.size() << std::endl;
    } else {
        std::cout << "DFS: Path not found." << std::endl;
        // Save the final state if no path is found (all visited cells will be marked)
        save_image(folder, step++, visual_states, maze_data); // Pass maze_data
    }
}


} // namespace MazeSolver