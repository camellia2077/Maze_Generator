#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <stack>
#include <algorithm>
#include <random>
#include <iomanip>    // For std::setw and std::setfill
#include <filesystem> // For creating directories (C++17)
#include <fstream>    // For INI file reading
#include <sstream>    // For parsing string to int

// Define STB_IMAGE_WRITE_IMPLEMENTATION in exactly one C or C++ file
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace fs = std::filesystem;

// --- Configuration Variables (will be loaded from INI or use defaults) ---
int MAZE_WIDTH = 10;    // Default
int MAZE_HEIGHT = 10;   // Default
int UNIT_PIXELS = 15;   // Default
std::pair<int, int> START_NODE = {0, 0}; // Default, can be overridden after loading config
std::pair<int, int> END_NODE = {MAZE_HEIGHT - 1, MAZE_WIDTH - 1}; // Default, will be updated after loading config

// --- Color Definitions (R, G, B) - mutable to be loaded from config ---
unsigned char COLOR_BACKGROUND[3] = {255, 255, 255}; // White (paths)
unsigned char COLOR_WALL[3] = {0, 0, 0};           // Black
unsigned char COLOR_START[3] = {0, 255, 0};        // Green
unsigned char COLOR_END[3] = {255, 0, 0};          // Red
unsigned char COLOR_FRONTIER[3] = {173, 216, 230}; // Light Blue (BFS queue, DFS stack)
unsigned char COLOR_VISITED[3] = {211, 211, 211};   // Light Gray
unsigned char COLOR_CURRENT[3] = {255, 165, 0};    // Orange
unsigned char COLOR_SOLUTION_PATH[3] = {0, 0, 255}; // Blue

// --- Maze Cell Structure ---
struct Cell {
    bool visited_gen = false; // For maze generation
    // Walls: Top, Right, Bottom, Left
    bool walls[4] = {true, true, true, true};
    std::pair<int, int> parent = {-1, -1}; // For path reconstruction in solvers
};

std::vector<std::vector<Cell>> maze;

// --- Solver Visual State Enum ---
enum class SolverCellState {
    NONE,
    START,
    END,
    FRONTIER,
    CURRENT_PROC, // Cell currently being processed
    VISITED_PROC, // Cell already processed/explored
    SOLUTION
};

// --- Utility function to trim whitespace ---
std::string trim(const std::string& str) {
    const std::string whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) return ""; // Return empty string if only whitespace
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

// --- Utility function to parse Hex color string (e.g., "#RRGGBB" or "RRGGBB") ---
bool parse_hex_color_string(std::string hex_str, unsigned char& r, unsigned char& g, unsigned char& b) {
    hex_str = trim(hex_str);
    if (hex_str.empty()) {
        std::cerr << "Warning: Empty hex color string provided.";
        return false;
    }

    if (hex_str[0] == '#') {
        hex_str = hex_str.substr(1);
    }

    if (hex_str.length() != 6) {
        std::cerr << "Warning: Hex color string '" << (hex_str.length() > 0 && hex_str[0] == '#' ? "" : "#") << hex_str << "' must be 6 characters long (excluding '#').";
        return false;
    }

    try {
        unsigned long value_r = std::stoul(hex_str.substr(0, 2), nullptr, 16);
        unsigned long value_g = std::stoul(hex_str.substr(2, 2), nullptr, 16);
        unsigned long value_b = std::stoul(hex_str.substr(4, 2), nullptr, 16);

        if (value_r > 255 || value_g > 255 || value_b > 255) {
             std::cerr << "Warning: Hex color component out of range (00-FF) in '" << (hex_str.length() > 0 && hex_str[0] == '#' ? "" : "#") << hex_str << "'.";
            return false;
        }
        r = static_cast<unsigned char>(value_r);
        g = static_cast<unsigned char>(value_g);
        b = static_cast<unsigned char>(value_b);
        return true;
    } catch (const std::invalid_argument& ia) {
        std::cerr << "Warning: Invalid character in hex color string '" << (hex_str.length() > 0 && hex_str[0] == '#' ? "" : "#") << hex_str << "'.";
        return false;
    } catch (const std::out_of_range& oor) {
        std::cerr << "Warning: Hex color value out of range in string '" << (hex_str.length() > 0 && hex_str[0] == '#' ? "" : "#") << hex_str << "'.";
        return false;
    }
}


// --- Function to load configuration from INI file ---
void load_config(const std::string& filename) {
    std::ifstream ini_file(filename);
    if (!ini_file.is_open()) {
        std::cerr << "Warning: Could not open config file '" << filename << "'. Using default values for all settings." << std::endl;
        END_NODE = {MAZE_HEIGHT - 1, MAZE_WIDTH - 1}; // Ensure END_NODE is based on defaults
        return;
    }

    std::string line;
    std::string current_section;
    bool in_maze_config_section = false;
    bool in_color_config_section = false;

    int temp_start_x = START_NODE.second;
    int temp_start_y = START_NODE.first;
    int temp_end_x = -1;
    int temp_end_y = -1;


    while (std::getline(ini_file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == ';') {
            continue;
        }

        if (line[0] == '[' && line.back() == ']') {
            current_section = trim(line.substr(1, line.length() - 2));
            in_maze_config_section = (current_section == "MazeConfig");
            in_color_config_section = (current_section == "ColorConfig");
            continue;
        }

        if (in_maze_config_section) {
            size_t delimiter_pos = line.find('=');
            if (delimiter_pos != std::string::npos) {
                std::string key = trim(line.substr(0, delimiter_pos));
                std::string value_str = trim(line.substr(delimiter_pos + 1));
                // Remove comments from maze config values as well, if any (though not typical for these values)
                size_t comment_pos = value_str.find(';');
                if (comment_pos != std::string::npos) {
                    value_str = trim(value_str.substr(0, comment_pos));
                }
                try {
                    int value_int = std::stoi(value_str);
                    if (key == "MazeWidth") MAZE_WIDTH = std::max(1, value_int);
                    else if (key == "MazeHeight") MAZE_HEIGHT = std::max(1, value_int);
                    else if (key == "UnitPixels") UNIT_PIXELS = std::max(1, value_int);
                    else if (key == "StartNodeX") temp_start_x = value_int;
                    else if (key == "StartNodeY") temp_start_y = value_int;
                    else if (key == "EndNodeX") temp_end_x = value_int;
                    else if (key == "EndNodeY") temp_end_y = value_int;
                } catch (const std::exception& e) {
                    std::cerr << "Warning: Invalid or out of range integer for key '" << key << "' in [MazeConfig]. Skipping. Error: " << e.what() << std::endl;
                }
            }
        } else if (in_color_config_section) {
            size_t delimiter_pos = line.find('=');
            if (delimiter_pos != std::string::npos) {
                std::string key = trim(line.substr(0, delimiter_pos));
                std::string value_str = trim(line.substr(delimiter_pos + 1));

                // MODIFICATION: Remove comment from color string before parsing
                size_t comment_pos = value_str.find(';');
                if (comment_pos != std::string::npos) {
                    value_str = trim(value_str.substr(0, comment_pos));
                }
                // END MODIFICATION

                unsigned char r, g, b;
                if (parse_hex_color_string(value_str, r, g, b)) {
                    if (key == "BackgroundColor") { COLOR_BACKGROUND[0]=r; COLOR_BACKGROUND[1]=g; COLOR_BACKGROUND[2]=b; }
                    else if (key == "WallColor") { COLOR_WALL[0]=r; COLOR_WALL[1]=g; COLOR_WALL[2]=b; }
                    else if (key == "StartNodeColor") { COLOR_START[0]=r; COLOR_START[1]=g; COLOR_START[2]=b; }
                    else if (key == "EndNodeColor") { COLOR_END[0]=r; COLOR_END[1]=g; COLOR_END[2]=b; }
                    else if (key == "FrontierColor") { COLOR_FRONTIER[0]=r; COLOR_FRONTIER[1]=g; COLOR_FRONTIER[2]=b; }
                    else if (key == "VisitedColor") { COLOR_VISITED[0]=r; COLOR_VISITED[1]=g; COLOR_VISITED[2]=b; }
                    else if (key == "CurrentProcessingColor") { COLOR_CURRENT[0]=r; COLOR_CURRENT[1]=g; COLOR_CURRENT[2]=b; }
                    else if (key == "SolutionPathColor") { COLOR_SOLUTION_PATH[0]=r; COLOR_SOLUTION_PATH[1]=g; COLOR_SOLUTION_PATH[2]=b; }
                    else { std::cerr << "Warning: Unknown color key '" << key << "' in [ColorConfig]. Skipping." << std::endl; }
                } else {
                    std::cerr << " Using default for '" << key << "'." << std::endl;
                }
            }
        }
    }
    ini_file.close();

    if (temp_start_x >= 0 && temp_start_x < MAZE_WIDTH && temp_start_y >= 0 && temp_start_y < MAZE_HEIGHT) {
        START_NODE = {temp_start_y, temp_start_x};
    } else {
        std::cerr << "Warning: Custom StartNode from INI is out of bounds for current MAZE_WIDTH/MAZE_HEIGHT. Using default (0,0)." << std::endl;
        START_NODE = {0,0};
    }
    
    int final_end_y = (temp_end_y != -1) ? temp_end_y : MAZE_HEIGHT - 1;
    int final_end_x = (temp_end_x != -1) ? temp_end_x : MAZE_WIDTH - 1;

    if (final_end_x >= 0 && final_end_x < MAZE_WIDTH && final_end_y >= 0 && final_end_y < MAZE_HEIGHT) {
        END_NODE = {final_end_y, final_end_x};
    } else {
        std::cerr << "Warning: Custom EndNode from INI is out of bounds or default is invalid after dimension change. Adjusting to (" 
                  << MAZE_HEIGHT -1 << "," << MAZE_WIDTH -1 << ")." << std::endl;
        END_NODE = {MAZE_HEIGHT - 1, MAZE_WIDTH - 1};
    }
    END_NODE.first = std::max(0, std::min(MAZE_HEIGHT - 1, END_NODE.first));
    END_NODE.second = std::max(0, std::min(MAZE_WIDTH - 1, END_NODE.second));

    if (MAZE_WIDTH > 1 || MAZE_HEIGHT > 1) {
        if (START_NODE == END_NODE) {
             std::cerr << "Warning: START_NODE and END_NODE are identical. This might lead to unexpected behavior." << std::endl;
        }
    }

    std::cout << "Configuration successfully " << std::endl;
}


// --- Maze Generation (Randomized DFS / Recursive Backtracker) ---
void generate_maze(int r, int c) {
    maze[r][c].visited_gen = true;
    std::vector<std::pair<int, int>> directions = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}}; // R, D, L, U
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(directions.begin(), directions.end(), g);

    for (const auto& dir : directions) {
        int nr = r + dir.first;
        int nc = c + dir.second;

        if (nr >= 0 && nr < MAZE_HEIGHT && nc >= 0 && nc < MAZE_WIDTH && !maze[nr][nc].visited_gen) {
            if (dir.first == 0 && dir.second == 1) { // Right
                maze[r][c].walls[1] = false;
                maze[nr][nc].walls[3] = false;
            } else if (dir.first == 1 && dir.second == 0) { // Down
                maze[r][c].walls[2] = false;
                maze[nr][nc].walls[0] = false;
            } else if (dir.first == 0 && dir.second == -1) { // Left
                maze[r][c].walls[3] = false;
                maze[nr][nc].walls[1] = false;
            } else if (dir.first == -1 && dir.second == 0) { // Up
                maze[r][c].walls[0] = false;
                maze[nr][nc].walls[2] = false;
            }
            generate_maze(nr, nc);
        }
    }
}

// --- Image Saving Function ---
void save_image(const std::string& folder, int step_count,
                const std::vector<std::vector<SolverCellState>>& visual_states,
                const std::vector<std::pair<int, int>>& current_path = {}) {

    if (MAZE_WIDTH <= 0 || MAZE_HEIGHT <= 0 || UNIT_PIXELS <= 0) {
        std::cerr << "Error: Invalid maze dimensions or unit pixels for saving image. Aborting save." << std::endl;
        return;
    }

    int img_width_units = 2 * MAZE_WIDTH + 1;
    int img_height_units = 2 * MAZE_HEIGHT + 1;
    int final_img_width = img_width_units * UNIT_PIXELS;
    int final_img_height = img_height_units * UNIT_PIXELS;

    std::vector<unsigned char> pixels(final_img_width * final_img_height * 3);

    for (int r_unit = 0; r_unit < img_height_units; ++r_unit) {
        for (int c_unit = 0; c_unit < img_width_units; ++c_unit) {
            const unsigned char* current_color_ptr = COLOR_WALL; // Default to wall

            if (r_unit % 2 != 0 && c_unit % 2 != 0) { // Cell interior
                int maze_r = (r_unit - 1) / 2;
                int maze_c = (c_unit - 1) / 2;
                current_color_ptr = COLOR_BACKGROUND; // Path part

                SolverCellState state = visual_states[maze_r][maze_c];
                if (state == SolverCellState::START) current_color_ptr = COLOR_START;
                else if (state == SolverCellState::END) current_color_ptr = COLOR_END;
                else if (state == SolverCellState::SOLUTION) current_color_ptr = COLOR_SOLUTION_PATH;
                else if (state == SolverCellState::CURRENT_PROC) current_color_ptr = COLOR_CURRENT;
                else if (state == SolverCellState::FRONTIER) current_color_ptr = COLOR_FRONTIER;
                else if (state == SolverCellState::VISITED_PROC) current_color_ptr = COLOR_VISITED;

                if (state != SolverCellState::SOLUTION) {
                     if (maze_r == START_NODE.first && maze_c == START_NODE.second) current_color_ptr = COLOR_START;
                     if (maze_r == END_NODE.first && maze_c == END_NODE.second) current_color_ptr = COLOR_END;
                }


            } else if (r_unit % 2 != 0 && c_unit % 2 == 0) { // Vertical wall potential
                int maze_r = (r_unit - 1) / 2;
                int maze_c_left = (c_unit / 2) - 1;
                if (c_unit > 0 && c_unit < 2 * MAZE_WIDTH) { // Check bounds before accessing maze
                    if (maze_r >= 0 && maze_r < MAZE_HEIGHT && maze_c_left >= 0 && maze_c_left < MAZE_WIDTH) {
                       if (!maze[maze_r][maze_c_left].walls[1]) current_color_ptr = COLOR_BACKGROUND;
                    }
                }
            } else if (r_unit % 2 == 0 && c_unit % 2 != 0) { // Horizontal wall potential
                int maze_r_up = (r_unit / 2) - 1;
                int maze_c = (c_unit - 1) / 2;
                 if (r_unit > 0 && r_unit < 2 * MAZE_HEIGHT) { // Check bounds before accessing maze
                    if (maze_r_up >= 0 && maze_r_up < MAZE_HEIGHT && maze_c >=0 && maze_c < MAZE_WIDTH) {
                        if (!maze[maze_r_up][maze_c].walls[2]) current_color_ptr = COLOR_BACKGROUND;
                    }
                }
            } else { // Pillar
                 current_color_ptr = COLOR_WALL;
            }

            for (int py = 0; py < UNIT_PIXELS; ++py) {
                for (int px = 0; px < UNIT_PIXELS; ++px) {
                    int img_x = c_unit * UNIT_PIXELS + px;
                    int img_y = r_unit * UNIT_PIXELS + py;
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
        const unsigned char* path_color_to_use_ptr = COLOR_SOLUTION_PATH;
        if (p == START_NODE) path_color_to_use_ptr = COLOR_START;
        else if (p == END_NODE) path_color_to_use_ptr = COLOR_END;

        for (int py = 0; py < UNIT_PIXELS; ++py) {
            for (int px = 0; px < UNIT_PIXELS; ++px) {
                int img_x = (2 * maze_c + 1) * UNIT_PIXELS + px;
                int img_y = (2 * maze_r + 1) * UNIT_PIXELS + py;
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
            const unsigned char* corridor_color_ptr = COLOR_SOLUTION_PATH;

            if (r1 == r2) { 
                corridor_r_unit = 2 * r1 + 1;
                corridor_c_unit = 2 * std::min(c1, c2) + 2;
            } else if (c1 == c2) { 
                corridor_c_unit = 2 * c1 + 1;
                corridor_r_unit = 2 * std::min(r1, r2) + 2;
            }

            if (corridor_r_unit != -1 && corridor_c_unit != -1) {
                for (int py = 0; py < UNIT_PIXELS; ++py) {
                    for (int px = 0; px < UNIT_PIXELS; ++px) {
                        int img_x = corridor_c_unit * UNIT_PIXELS + px;
                        int img_y = corridor_r_unit * UNIT_PIXELS + py;
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
    // p1 is grandparent, p2 is parent, p3 is current
    if (p1.first == -1 || p1.second == -1 ||
        p2.first == -1 || p2.second == -1 ||
        p3.first == -1 || p3.second == -1) {
        return false; // Not enough points to form a line segment
    }

    // Check if direction from p1 to p2 is the same as from p2 to p3
    int dr1 = p2.first - p1.first;
    int dc1 = p2.second - p1.second;
    int dr2 = p3.first - p2.first;
    int dc2 = p3.second - p2.second;

    // Since these are adjacent cells in a grid, a straight line means the step direction is identical
    return dr1 == dr2 && dc1 == dc2;
}


// --- BFS Solver ---
void solve_bfs() {
    std::cout << "Starting BFS..." << std::endl;
    std::string folder = "bfs_frames";
    fs::create_directories(folder); 

    if (MAZE_WIDTH <=0 || MAZE_HEIGHT <=0) {
        std::cout << "BFS: Invalid maze dimensions. Aborting." << std::endl;
        return;
    }

    std::vector<std::vector<SolverCellState>> visual_states(MAZE_HEIGHT, std::vector<SolverCellState>(MAZE_WIDTH, SolverCellState::NONE));
    std::vector<std::vector<bool>> visited_solver(MAZE_HEIGHT, std::vector<bool>(MAZE_WIDTH, false));
    std::queue<std::pair<int, int>> q;

    for(int r=0; r<MAZE_HEIGHT; ++r) {
        for(int c=0; c<MAZE_WIDTH; ++c) {
            maze[r][c].parent = {-1, -1}; 
        }
    }

    q.push(START_NODE);
    visited_solver[START_NODE.first][START_NODE.second] = true;
    visual_states[START_NODE.first][START_NODE.second] = SolverCellState::FRONTIER;
    int step = 0;
    save_image(folder, step++, visual_states); // Save initial state

    std::vector<std::pair<int, int>> path;
    bool found = false;

    int dr[] = {-1, 1, 0, 0}; 
    int dc[] = {0, 0, -1, 1}; 

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
        // Always save frames if we are processing the end node or start node (start node handled by initial save)
        if (curr == END_NODE) {
            should_save_current_step_frames = true;
        }

        visual_states[curr.first][curr.second] = SolverCellState::CURRENT_PROC;
        if (should_save_current_step_frames) {
            save_image(folder, step++, visual_states);
        }

        if (curr == END_NODE) {
            found = true;
            // No break here, let it save the "VISITED_PROC" state for the end node if frames are being saved
        }

        // Only explore neighbors if not already found. If found, we just want to finalize this cell's visuals.
        if (!found) {
            for (int i = 0; i < 4; ++i) {
                int nr = curr.first + dr[i];
                int nc = curr.second + dc[i];
                bool wall_exists = true;
                // Determine wall presence based on direction 'i'
                if (dr[i] == -1 && dc[i] == 0 && !maze[curr.first][curr.second].walls[0]) wall_exists = false; // Up
                else if (dr[i] == 1  && dc[i] == 0 && !maze[curr.first][curr.second].walls[2]) wall_exists = false; // Down
                else if (dr[i] == 0 && dc[i] == -1 && !maze[curr.first][curr.second].walls[3]) wall_exists = false; // Left
                else if (dr[i] == 0 && dc[i] == 1  && !maze[curr.first][curr.second].walls[1]) wall_exists = false; // Right


                if (nr >= 0 && nr < MAZE_HEIGHT && nc >= 0 && nc < MAZE_WIDTH &&
                    !visited_solver[nr][nc] && !wall_exists) {
                    
                    visited_solver[nr][nc] = true;
                    maze[nr][nc].parent = curr;
                    q.push({nr, nc});
                    visual_states[nr][nc] = SolverCellState::FRONTIER;
                }
            }
        }
        
        visual_states[curr.first][curr.second] = SolverCellState::VISITED_PROC; 
        if (should_save_current_step_frames) {
            save_image(folder, step++, visual_states);
        }
        if (found && curr == END_NODE) break; // Now break after saving end_node as visited
    }

    if (found) {
        std::pair<int, int> curr_path_node = END_NODE;
        while (curr_path_node.first != -1 && curr_path_node.second != -1) {
            path.push_back(curr_path_node);
            visual_states[curr_path_node.first][curr_path_node.second] = SolverCellState::SOLUTION;
            if (curr_path_node == START_NODE) break; 
            curr_path_node = maze[curr_path_node.first][curr_path_node.second].parent;
        }
        std::reverse(path.begin(), path.end());
         save_image(folder, step++, visual_states, path); // Save final path
        std::cout << "BFS: Path found. Length: " << path.size() << std::endl;
    } else {
        std::cout << "BFS: Path not found." << std::endl;
        save_image(folder, step++, visual_states); // Save final state if not found
    }
}

// --- DFS Solver ---
void solve_dfs() {
    std::cout << "Starting DFS..." << std::endl;
    std::string folder = "dfs_frames";
    fs::create_directories(folder);

    if (MAZE_WIDTH <=0 || MAZE_HEIGHT <=0) {
        std::cout << "DFS: Invalid maze dimensions. Aborting." << std::endl;
        return;
    }

    std::vector<std::vector<SolverCellState>> visual_states(MAZE_HEIGHT, std::vector<SolverCellState>(MAZE_WIDTH, SolverCellState::NONE));
    std::vector<std::vector<bool>> visited_solver(MAZE_HEIGHT, std::vector<bool>(MAZE_WIDTH, false));
    std::stack<std::pair<int, int>> s;
    
    for(int r=0; r<MAZE_HEIGHT; ++r) {
        for(int c=0; c<MAZE_WIDTH; ++c) {
            maze[r][c].parent = {-1, -1}; 
        }
    }

    s.push(START_NODE);
    int step = 0;
    // Initial frontier setup for DFS
    visual_states[START_NODE.first][START_NODE.second] = SolverCellState::FRONTIER;
    save_image(folder, step++, visual_states);


    std::vector<std::pair<int, int>> path;
    bool found = false;

    int dr[] = {-1, 1, 0, 0}; 
    int dc[] = {0, 0, -1, 1};

    while (!s.empty() && !found) {
        std::pair<int, int> curr = s.top();
        // Don't pop yet, decide if we need to save based on curr.
        // If curr was already visited (e.g. by another path in DFS or if it's popped again after exploring children),
        // this logic might need adjustment. For now, assume curr is being "considered".

        bool should_save_current_step_frames = true;
        std::pair<int, int> parent_cell = maze[curr.first][curr.second].parent; // Parent is set when pushed
        
        // For DFS, the "parent" for straight line check might be the one *before* it on the stack,
        // or more consistently, its actual parent from the `maze[r][c].parent` field,
        // which is set when a node is *pushed* as a child of another.
        if (parent_cell.first != -1 && parent_cell.second != -1) {
            std::pair<int, int> grandparent_cell = maze[parent_cell.first][parent_cell.second].parent;
            if (grandparent_cell.first != -1 && grandparent_cell.second != -1) {
                if (is_straight_line(grandparent_cell, parent_cell, curr)) {
                    should_save_current_step_frames = false;
                }
            }
        }
        if (curr == END_NODE) {
             should_save_current_step_frames = true;
        }
        // If curr is START_NODE, it's handled by initial save.
        // Subsequent visits to START_NODE (if possible by algo design) would have no parent, so save.


        // --- First potential save point for DFS step ---
        // This shows the current cell being focused on and the current state of the stack (frontier)
        // Clear previous frontiers that are not on stack anymore for DFS visualization
        for(int r_idx=0; r_idx<MAZE_HEIGHT; ++r_idx) {
            for(int c_idx=0; c_idx<MAZE_WIDTH; ++c_idx) {
                if(visual_states[r_idx][c_idx] == SolverCellState::FRONTIER) {
                    bool is_on_stack = false;
                    std::stack<std::pair<int,int>> temp_check_stack = s;
                    while(!temp_check_stack.empty()){
                        if(temp_check_stack.top().first == r_idx && temp_check_stack.top().second == c_idx) {
                            is_on_stack = true;
                            break;
                        }
                        temp_check_stack.pop();
                    }
                    if (!is_on_stack) visual_states[r_idx][c_idx] = SolverCellState::NONE;
                }
            }
        }
        std::stack<std::pair<int,int>> s_copy_for_frontier_vis = s;
        while(!s_copy_for_frontier_vis.empty()){ // Mark current stack items as frontier
             if(visual_states[s_copy_for_frontier_vis.top().first][s_copy_for_frontier_vis.top().second] != SolverCellState::CURRENT_PROC &&
                visual_states[s_copy_for_frontier_vis.top().first][s_copy_for_frontier_vis.top().second] != SolverCellState::VISITED_PROC &&
                !visited_solver[s_copy_for_frontier_vis.top().first][s_copy_for_frontier_vis.top().second] ) { // only if not yet processed/visited
                 visual_states[s_copy_for_frontier_vis.top().first][s_copy_for_frontier_vis.top().second] = SolverCellState::FRONTIER;
             }
            s_copy_for_frontier_vis.pop();
        }
        // Now that frontier is updated, decide on saving
        visual_states[curr.first][curr.second] = SolverCellState::CURRENT_PROC; // curr is being processed
        if (should_save_current_step_frames) {
            save_image(folder, step++, visual_states);
        }
        
        s.pop(); // Actually pop the element now

        if (visited_solver[curr.first][curr.second] && curr != END_NODE) { // If already visited and not the end node, just skip (don't re-process)
            // If we skipped saving its CURRENT_PROC but it was visited, mark it VISITED_PROC for consistency
            // if (visual_states[curr.first][curr.second] == SolverCellState::CURRENT_PROC){ // if it was marked current
            //      visual_states[curr.first][curr.second] = SolverCellState::VISITED_PROC;
            // } // This might cause issues with re-coloring. Simpler: if visited, continue.
            continue;
        }
        
        visited_solver[curr.first][curr.second] = true;
        // visual_states[curr.first][curr.second] already CURRENT_PROC or updated by save

        if (curr == END_NODE) {
            found = true;
            // No break yet, let the "VISITED_PROC" state be saved if frames are saved for this step.
        }

        if (!found) {
            for (int i = 0; i < 4; ++i) { 
                int nr = curr.first + dr[i];
                int nc = curr.second + dc[i];
                bool wall_exists = true;
                if (dr[i] == -1 && dc[i] == 0 && !maze[curr.first][curr.second].walls[0]) wall_exists = false; // Up
                else if (dr[i] == 1  && dc[i] == 0 && !maze[curr.first][curr.second].walls[2]) wall_exists = false; // Down
                else if (dr[i] == 0 && dc[i] == -1 && !maze[curr.first][curr.second].walls[3]) wall_exists = false; // Left
                else if (dr[i] == 0 && dc[i] == 1  && !maze[curr.first][curr.second].walls[1]) wall_exists = false; // Right


                if (nr >= 0 && nr < MAZE_HEIGHT && nc >= 0 && nc < MAZE_WIDTH &&
                    !visited_solver[nr][nc] && !wall_exists) {
                    maze[nr][nc].parent = curr;
                    s.push({nr, nc});
                    // visual_states[nr][nc] will be marked FRONTIER in the next iteration's stack scan, or if saved now
                }
            }
        }
        
        // --- Second potential save point for DFS step ---
        // Shows curr as visited, and new stack (if any children pushed) as frontier
        visual_states[curr.first][curr.second] = SolverCellState::VISITED_PROC;
        
        // Update frontier visualization based on the new stack state
        for(int r_idx=0; r_idx<MAZE_HEIGHT; ++r_idx) { // Clear non-stack frontiers again
            for(int c_idx=0; c_idx<MAZE_WIDTH; ++c_idx) {
                 if(visual_states[r_idx][c_idx] == SolverCellState::FRONTIER) {
                    bool is_on_stack = false;
                    std::stack<std::pair<int,int>> temp_check_stack = s;
                    while(!temp_check_stack.empty()){
                        if(temp_check_stack.top().first == r_idx && temp_check_stack.top().second == c_idx) {
                            is_on_stack = true;
                            break;
                        }
                        temp_check_stack.pop();
                    }
                    if (!is_on_stack && !(r_idx == curr.first && c_idx == curr.second)) visual_states[r_idx][c_idx] = SolverCellState::NONE;
                }
            }
        }
        s_copy_for_frontier_vis = s; 
        while(!s_copy_for_frontier_vis.empty()){ // Mark new stack items as frontier
            if(visual_states[s_copy_for_frontier_vis.top().first][s_copy_for_frontier_vis.top().second] != SolverCellState::CURRENT_PROC &&
               visual_states[s_copy_for_frontier_vis.top().first][s_copy_for_frontier_vis.top().second] != SolverCellState::VISITED_PROC &&
                !visited_solver[s_copy_for_frontier_vis.top().first][s_copy_for_frontier_vis.top().second] ) {
                 visual_states[s_copy_for_frontier_vis.top().first][s_copy_for_frontier_vis.top().second] = SolverCellState::FRONTIER;
            }
            s_copy_for_frontier_vis.pop();
        }
        if (should_save_current_step_frames) {
            save_image(folder, step++, visual_states);
        }
        if (found && curr == END_NODE) break; // Break after saving end_node as visited
    }

    if (found) {
        std::pair<int, int> curr_path_node = END_NODE;
        while (curr_path_node.first != -1 && curr_path_node.second != -1) {
            path.push_back(curr_path_node);
            visual_states[curr_path_node.first][curr_path_node.second] = SolverCellState::SOLUTION;
             if (curr_path_node == START_NODE) break;
            curr_path_node = maze[curr_path_node.first][curr_path_node.second].parent;
        }
        std::reverse(path.begin(), path.end());
        save_image(folder, step++, visual_states, path); // Save final path
        std::cout << "DFS: Path found. Length: " << path.size() << std::endl;
    } else {
        std::cout << "DFS: Path not found." << std::endl;
         save_image(folder, step++, visual_states); // Save final state if not found
    }
}


int main() {
    load_config("config.ini"); 

    if (MAZE_WIDTH <= 0 || MAZE_HEIGHT <= 0) {
        std::cerr << "Error: MAZE_WIDTH and MAZE_HEIGHT must be greater than 0. Exiting." << std::endl;
        return 1;
    }
    if (UNIT_PIXELS <= 0) {
        std::cerr << "Error: UNIT_PIXELS must be greater than 0. Exiting." << std::endl;
        return 1;
    }


    maze.resize(MAZE_HEIGHT, std::vector<Cell>(MAZE_WIDTH));

    std::cout << "Generating maze..." << std::endl;
    int gen_start_r = (START_NODE.first >=0 && START_NODE.first < MAZE_HEIGHT) ? START_NODE.first : 0;
    int gen_start_c = (START_NODE.second >=0 && START_NODE.second < MAZE_WIDTH) ? START_NODE.second : 0;
    if (gen_start_r != START_NODE.first || gen_start_c != START_NODE.second) {
        std::cout << "Adjusted maze generation start point to (" << gen_start_r << "," << gen_start_c << ") due to START_NODE config." << std::endl;
    }

    generate_maze(gen_start_r, gen_start_c); 
    std::cout << "Maze generated." << std::endl;

    solve_bfs();
    // Reset maze parents for DFS if BFS modified them and DFS relies on clean parent fields for its own path reconstruction
    for(int r=0; r<MAZE_HEIGHT; ++r) {
        for(int c=0; c<MAZE_WIDTH; ++c) {
            maze[r][c].parent = {-1, -1}; 
        }
    }
    solve_dfs();

    std::cout << "Processing complete. Images saved in bfs_frames/ and dfs_frames/." << std::endl;

    return 0;
}