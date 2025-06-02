#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <stack>
#include <algorithm>
#include <random>
#include <iomanip>
#include <filesystem>

#include "maze_generation_module.h"
#include "config_loader.h" // 包含新的配置加载器头文件

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace fs = std::filesystem;

// --- Maze Cell Structure for the main program (solver-focused) ---
struct Cell {
    bool walls[4] = {true, true, true, true}; // Top, Right, Bottom, Left
    std::pair<int, int> parent = {-1, -1};
};

std::vector<std::vector<Cell>> maze;

// --- Solver Visual State Enum ---
enum class SolverCellState {
    NONE, START, END, FRONTIER, CURRENT_PROC, VISITED_PROC, SOLUTION
};


// --- Image Saving Function ---
// 此函数现在使用 ConfigLoader::MAZE_WIDTH, ConfigLoader::COLOR_WALL 等。
void save_image(const std::string& folder, int step_count,
                const std::vector<std::vector<SolverCellState>>& visual_states,
                const std::vector<std::pair<int, int>>& current_path = {}) {

    // 使用 ConfigLoader:: 获取全局配置变量
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
            const unsigned char* current_color_ptr = ConfigLoader::COLOR_WALL;

            if (r_unit % 2 != 0 && c_unit % 2 != 0) {
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


            } else if (r_unit % 2 != 0 && c_unit % 2 == 0) { // Vertical Wall location
                int maze_r = (r_unit - 1) / 2;
                int maze_c_left = (c_unit / 2) - 1;
                if (c_unit > 0 && c_unit < 2 * ConfigLoader::MAZE_WIDTH) {
                    if (maze_r >= 0 && maze_r < ConfigLoader::MAZE_HEIGHT && maze_c_left >= 0 && maze_c_left < ConfigLoader::MAZE_WIDTH) {
                       if (!maze[maze_r][maze_c_left].walls[1]) current_color_ptr = ConfigLoader::COLOR_BACKGROUND; // Check right wall of left cell
                    }
                }
            } else if (r_unit % 2 == 0 && c_unit % 2 != 0) { // Horizontal Wall location
                int maze_r_up = (r_unit / 2) - 1;
                int maze_c = (c_unit - 1) / 2;
                 if (r_unit > 0 && r_unit < 2 * ConfigLoader::MAZE_HEIGHT) {
                    if (maze_r_up >= 0 && maze_r_up < ConfigLoader::MAZE_HEIGHT && maze_c >=0 && maze_c < ConfigLoader::MAZE_WIDTH) {
                        if (!maze[maze_r_up][maze_c].walls[2]) current_color_ptr = ConfigLoader::COLOR_BACKGROUND; // Check bottom wall of up cell
                    }
                }
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

    if (current_path.size() > 1) {
        for (size_t i = 0; i < current_path.size() - 1; ++i) {
            std::pair<int, int> p1 = current_path[i];
            std::pair<int, int> p2 = current_path[i+1];

            int r1 = p1.first; int c1 = p1.second;
            int r2 = p2.first; int c2 = p2.second;

            int corridor_r_unit = -1, corridor_c_unit = -1;
            const unsigned char* corridor_color_ptr = ConfigLoader::COLOR_SOLUTION_PATH;

            if (r1 == r2) {
                corridor_r_unit = 2 * r1 + 1;
                corridor_c_unit = 2 * std::min(c1, c2) + 2;
            } else if (c1 == c2) {
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


// --- BFS Solver ---
void solve_bfs() {
    std::string folder = "bfs_frames_generated_by_" + ConfigLoader::CURRENT_GENERATION_ALGORITHM_NAME_FOR_FOLDER;
    fs::create_directories(folder);
    std::cout << "Starting BFS for maze generated by " << ConfigLoader::CURRENT_GENERATION_ALGORITHM_NAME_FOR_FOLDER << " (frames in " << folder << ")..." << std::endl;

    if (ConfigLoader::MAZE_WIDTH <=0 || ConfigLoader::MAZE_HEIGHT <=0) {
        std::cout << "BFS: Invalid maze dimensions. Aborting." << std::endl;
        return;
    }
     if (ConfigLoader::START_NODE == ConfigLoader::END_NODE) {
        std::cout << "BFS: Start and End nodes are the same. Path is trivial." << std::endl;
        std::vector<std::vector<SolverCellState>> visual_states(ConfigLoader::MAZE_HEIGHT, std::vector<SolverCellState>(ConfigLoader::MAZE_WIDTH, SolverCellState::NONE));
        visual_states[ConfigLoader::START_NODE.first][ConfigLoader::START_NODE.second] = SolverCellState::SOLUTION;
        save_image(folder, 0, visual_states, {{ConfigLoader::START_NODE}});
        return;
    }

    std::vector<std::vector<SolverCellState>> visual_states(ConfigLoader::MAZE_HEIGHT, std::vector<SolverCellState>(ConfigLoader::MAZE_WIDTH, SolverCellState::NONE));
    std::vector<std::vector<bool>> visited_solver(ConfigLoader::MAZE_HEIGHT, std::vector<bool>(ConfigLoader::MAZE_WIDTH, false));
    std::queue<std::pair<int, int>> q;

    for(int r=0; r<ConfigLoader::MAZE_HEIGHT; ++r) {
        for(int c=0; c<ConfigLoader::MAZE_WIDTH; ++c) {
            maze[r][c].parent = {-1, -1};
        }
    }

    q.push(ConfigLoader::START_NODE);
    visited_solver[ConfigLoader::START_NODE.first][ConfigLoader::START_NODE.second] = true;
    visual_states[ConfigLoader::START_NODE.first][ConfigLoader::START_NODE.second] = SolverCellState::FRONTIER;
    int step = 0;
    save_image(folder, step++, visual_states);

    std::vector<std::pair<int, int>> path;
    bool found = false;

    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};
    int wall_check_indices[] = {0, 2, 3, 1};

    while (!q.empty() && !found) {
        std::pair<int, int> curr = q.front();
        q.pop();

        bool should_save_current_step_frames = true;
        std::pair<int, int> parent_cell = maze[curr.first][curr.second].parent;
        if (parent_cell.first != -1 && parent_cell.second != -1) {
            std::pair<int, int> grandparent_cell = maze[parent_cell.first][parent_cell.second].parent;
            if (grandparent_cell.first != -1 && grandparent_cell.second != -1) {
                if (is_straight_line(grandparent_cell, parent_cell, curr)) {
                    should_save_current_step_frames = false;
                }
            }
        }
        if (curr == ConfigLoader::END_NODE) {
            should_save_current_step_frames = true;
        }

        visual_states[curr.first][curr.second] = SolverCellState::CURRENT_PROC;
        if (should_save_current_step_frames) {
            save_image(folder, step++, visual_states);
        }

        if (curr == ConfigLoader::END_NODE) {
            found = true;
        }

        if (!found) {
            for (int i = 0; i < 4; ++i) {
                int nr = curr.first + dr[i];
                int nc = curr.second + dc[i];

                bool wall_exists = maze[curr.first][curr.second].walls[wall_check_indices[i]];

                if (nr >= 0 && nr < ConfigLoader::MAZE_HEIGHT && nc >= 0 && nc < ConfigLoader::MAZE_WIDTH &&
                    !visited_solver[nr][nc] && !wall_exists) {

                    visited_solver[nr][nc] = true;
                    maze[nr][nc].parent = curr;
                    q.push({nr, nc});
                    visual_states[nr][nc] = SolverCellState::FRONTIER;
                }
            }
        }

        if (curr != ConfigLoader::END_NODE || !found) {
             visual_states[curr.first][curr.second] = SolverCellState::VISITED_PROC;
        }

        if (should_save_current_step_frames) {
            save_image(folder, step++, visual_states);
        }
        if (found && curr == ConfigLoader::END_NODE) break;
    }

    if (found) {
        std::pair<int, int> curr_path_node = ConfigLoader::END_NODE;
        while (curr_path_node.first != -1 && curr_path_node.second != -1) {
            path.push_back(curr_path_node);
            visual_states[curr_path_node.first][curr_path_node.second] = SolverCellState::SOLUTION;
            if (curr_path_node == ConfigLoader::START_NODE) break;
            curr_path_node = maze[curr_path_node.first][curr_path_node.second].parent;
        }
        std::reverse(path.begin(), path.end());
         save_image(folder, step++, visual_states, path);
        std::cout << "BFS: Path found. Length: " << path.size() << std::endl;
    } else {
        std::cout << "BFS: Path not found." << std::endl;
        save_image(folder, step++, visual_states);
    }
}

// --- DFS Solver ---
void solve_dfs() {
    std::string folder = "dfs_frames_generated_by_" + ConfigLoader::CURRENT_GENERATION_ALGORITHM_NAME_FOR_FOLDER;
    fs::create_directories(folder);
    std::cout << "Starting DFS for maze generated by " << ConfigLoader::CURRENT_GENERATION_ALGORITHM_NAME_FOR_FOLDER << " (frames in " << folder << ")..." << std::endl;


    if (ConfigLoader::MAZE_WIDTH <=0 || ConfigLoader::MAZE_HEIGHT <=0) {
        std::cout << "DFS: Invalid maze dimensions. Aborting." << std::endl;
        return;
    }
    if (ConfigLoader::START_NODE == ConfigLoader::END_NODE) {
        std::cout << "DFS: Start and End nodes are the same. Path is trivial." << std::endl;
        std::vector<std::vector<SolverCellState>> visual_states(ConfigLoader::MAZE_HEIGHT, std::vector<SolverCellState>(ConfigLoader::MAZE_WIDTH, SolverCellState::NONE));
        visual_states[ConfigLoader::START_NODE.first][ConfigLoader::START_NODE.second] = SolverCellState::SOLUTION;
        save_image(folder, 0, visual_states, {{ConfigLoader::START_NODE}});
        return;
    }

    std::vector<std::vector<SolverCellState>> visual_states(ConfigLoader::MAZE_HEIGHT, std::vector<SolverCellState>(ConfigLoader::MAZE_WIDTH, SolverCellState::NONE));
    std::vector<std::vector<bool>> visited_solver(ConfigLoader::MAZE_HEIGHT, std::vector<bool>(ConfigLoader::MAZE_WIDTH, false));
    std::stack<std::pair<int, int>> s;

    for(int r=0; r<ConfigLoader::MAZE_HEIGHT; ++r) {
        for(int c=0; c<ConfigLoader::MAZE_WIDTH; ++c) {
            maze[r][c].parent = {-1, -1};
        }
    }

    s.push(ConfigLoader::START_NODE);
    int step = 0;
    visual_states[ConfigLoader::START_NODE.first][ConfigLoader::START_NODE.second] = SolverCellState::FRONTIER;
    save_image(folder, step++, visual_states);

    std::vector<std::pair<int, int>> path;
    bool found = false;

    int dr[] = {-1, 0, 1, 0};
    int dc[] = {0, 1, 0, -1};
    int wall_check_indices[] = {0, 1, 2, 3};

    while (!s.empty() && !found) {
        std::pair<int, int> curr = s.top();

        bool should_save_frame_for_curr = true;
        if(visual_states[curr.first][curr.second] == SolverCellState::VISITED_PROC) {
             s.pop();
             should_save_frame_for_curr = false;
             continue;
        }

        if (!visited_solver[curr.first][curr.second]) {
             visited_solver[curr.first][curr.second] = true;
             visual_states[curr.first][curr.second] = SolverCellState::CURRENT_PROC;

            std::pair<int, int> parent_cell = maze[curr.first][curr.second].parent;
            if (parent_cell.first != -1) {
                std::pair<int, int> grandparent_cell = maze[parent_cell.first][parent_cell.second].parent;
                if (grandparent_cell.first != -1) {
                    if (is_straight_line(grandparent_cell, parent_cell, curr)) {
                        should_save_frame_for_curr = false;
                    }
                }
            }
             if (curr == ConfigLoader::END_NODE) should_save_frame_for_curr = true;

            if (should_save_frame_for_curr) {
                save_image(folder, step++, visual_states);
            }
        }

        if (curr == ConfigLoader::END_NODE) {
            found = true;
            break;
        }

        bool pushed_neighbor = false;
        for (int i = 0; i < 4; ++i) {
            int nr = curr.first + dr[i];
            int nc = curr.second + dc[i];

            bool wall_exists = maze[curr.first][curr.second].walls[wall_check_indices[i]];

            if (nr >= 0 && nr < ConfigLoader::MAZE_HEIGHT && nc >= 0 && nc < ConfigLoader::MAZE_WIDTH &&
                !visited_solver[nr][nc] && !wall_exists) {

                maze[nr][nc].parent = curr;
                s.push({nr, nc});
                visual_states[nr][nc] = SolverCellState::FRONTIER;
                pushed_neighbor = true;
            }
        }

        if (!pushed_neighbor) {
            s.pop();
            visual_states[curr.first][curr.second] = SolverCellState::VISITED_PROC;
            if (should_save_frame_for_curr || curr == ConfigLoader::START_NODE) {
                bool re_save = true;
                std::pair<int, int> parent_cell = maze[curr.first][curr.second].parent;
                if(parent_cell.first != -1) {
                    std::pair<int, int> grandparent_cell = maze[parent_cell.first][parent_cell.second].parent;
                    if(grandparent_cell.first != -1 && is_straight_line(grandparent_cell, parent_cell, curr) && curr != ConfigLoader::END_NODE) {
                        re_save = false;
                    }
                }
                if(re_save) save_image(folder, step++, visual_states);
            }
        } else {
            if (visual_states[curr.first][curr.second] == SolverCellState::CURRENT_PROC && should_save_frame_for_curr) {
            }
        }
    }

    if (found) {
        std::pair<int, int> curr_path_node = ConfigLoader::END_NODE;
        while (curr_path_node.first != -1 && curr_path_node.second != -1) {
            path.push_back(curr_path_node);
            visual_states[curr_path_node.first][curr_path_node.second] = SolverCellState::SOLUTION;
             if (curr_path_node == ConfigLoader::START_NODE) break;
            curr_path_node = maze[curr_path_node.first][curr_path_node.second].parent;
        }
        std::reverse(path.begin(), path.end());
        save_image(folder, step++, visual_states, path);
        std::cout << "DFS: Path found. Length: " << path.size() << std::endl;
    } else {
        std::cout << "DFS: Path not found." << std::endl;
        save_image(folder, step++, visual_states);
    }
}


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

    // 遍历 AlgorithmInfo 结构体
    for (const auto& algo_info : ConfigLoader::ACTIVE_GENERATION_ALGORITHMS) {
        // 直接使用 algo_info.name 设置文件夹名称
        ConfigLoader::CURRENT_GENERATION_ALGORITHM_NAME_FOR_FOLDER = algo_info.name;

        std::cout << "\n--- Processing for Maze Generation Algorithm: "
                  << ConfigLoader::CURRENT_GENERATION_ALGORITHM_NAME_FOR_FOLDER << " ---" << std::endl;

        generation_maze_data.assign(ConfigLoader::MAZE_HEIGHT, std::vector<MazeGeneration::GenCell>(ConfigLoader::MAZE_WIDTH));

        std::cout << "Generating maze structure..." << std::endl;
        int gen_start_r = (ConfigLoader::START_NODE.first >= 0 && ConfigLoader::START_NODE.first < ConfigLoader::MAZE_HEIGHT) ? ConfigLoader::START_NODE.first : 0;
        int gen_start_c = (ConfigLoader::START_NODE.second >= 0 && ConfigLoader::START_NODE.second < ConfigLoader::MAZE_WIDTH) ? ConfigLoader::START_NODE.second : 0;

        if (!(ConfigLoader::START_NODE.first >= 0 && ConfigLoader::START_NODE.first < ConfigLoader::MAZE_HEIGHT &&
              ConfigLoader::START_NODE.second >= 0 && ConfigLoader::START_NODE.second < ConfigLoader::MAZE_WIDTH)) {
             // 使用 algo_info.type 来判断是否需要打印此警告
             if (algo_info.type == MazeGeneration::MazeAlgorithmType::DFS || algo_info.type == MazeGeneration::MazeAlgorithmType::PRIMS) {
                std::cout << "Adjusted maze generation start point to (" << gen_start_r << "," << gen_start_c
                          << ") due to out-of-bounds config START_NODE for DFS/Prims." << std::endl;
             }
        }

        // 使用 algo_info.type 来调用生成函数
        MazeGeneration::generate_maze_structure(generation_maze_data, gen_start_r, gen_start_c, ConfigLoader::MAZE_WIDTH, ConfigLoader::MAZE_HEIGHT, algo_info.type);

        maze.assign(ConfigLoader::MAZE_HEIGHT, std::vector<Cell>(ConfigLoader::MAZE_WIDTH));
        for (int r = 0; r < ConfigLoader::MAZE_HEIGHT; ++r) {
            for (int c = 0; c < ConfigLoader::MAZE_WIDTH; ++c) {
                for (int i = 0; i < 4; ++i) {
                    maze[r][c].walls[i] = generation_maze_data[r][c].walls[i];
                }
            }
        }
        std::cout << "Maze generated and data transferred." << std::endl;

        solve_bfs();
        solve_dfs();
    }

    std::cout << "\nProcessing complete for all selected algorithms. Images saved in respective folders." << std::endl;

    return 0;
}