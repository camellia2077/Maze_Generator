#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <stack>
#include <algorithm>
#include <random> 
#include <iomanip>    
#include <filesystem> 
#include <fstream>    
#include <sstream>    

#include "maze_generation_module.h" 

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace fs = std::filesystem;

// --- Configuration Variables ---
int MAZE_WIDTH = 10;    
int MAZE_HEIGHT = 10;   
int UNIT_PIXELS = 15;   
std::pair<int, int> START_NODE = {0, 0};
std::pair<int, int> END_NODE = {MAZE_HEIGHT - 1, MAZE_WIDTH - 1};
std::vector<MazeGeneration::MazeAlgorithmType> ACTIVE_GENERATION_ALGORITHMS; // To be filled by load_config
std::string CURRENT_GENERATION_ALGORITHM_NAME_FOR_FOLDER; // For naming output folders

// --- Color Definitions (R, G, B) ---
unsigned char COLOR_BACKGROUND[3] = {255, 255, 255};
unsigned char COLOR_WALL[3] = {0, 0, 0};
unsigned char COLOR_START[3] = {0, 255, 0};
unsigned char COLOR_END[3] = {255, 0, 0};
unsigned char COLOR_FRONTIER[3] = {173, 216, 230};
unsigned char COLOR_VISITED[3] = {211, 211, 211};
unsigned char COLOR_CURRENT[3] = {255, 165, 0};
unsigned char COLOR_SOLUTION_PATH[3] = {0, 0, 255}; // Was blue, changed for visibility from config: #0099CC

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

// --- Utility function to trim whitespace ---
std::string trim(const std::string& str) {
    const std::string whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) return ""; 
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

// --- Utility function to parse Hex color string ---
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
        END_NODE = {MAZE_HEIGHT - 1, MAZE_WIDTH - 1}; 
        ACTIVE_GENERATION_ALGORITHMS.push_back(MazeGeneration::MazeAlgorithmType::DFS); // Default algo
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
    std::string algo_list_str;


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

        size_t delimiter_pos = line.find('=');
        if (delimiter_pos == std::string::npos) continue;

        std::string key = trim(line.substr(0, delimiter_pos));
        std::string value_str = trim(line.substr(delimiter_pos + 1));
        size_t comment_pos = value_str.find(';');
        if (comment_pos != std::string::npos) {
            value_str = trim(value_str.substr(0, comment_pos));
        }

        if (in_maze_config_section) {
            try {
                if (key == "MazeWidth") MAZE_WIDTH = std::max(1, std::stoi(value_str));
                else if (key == "MazeHeight") MAZE_HEIGHT = std::max(1, std::stoi(value_str));
                else if (key == "UnitPixels") UNIT_PIXELS = std::max(1, std::stoi(value_str));
                else if (key == "StartNodeX") temp_start_x = std::stoi(value_str);
                else if (key == "StartNodeY") temp_start_y = std::stoi(value_str);
                else if (key == "EndNodeX") temp_end_x = std::stoi(value_str);
                else if (key == "EndNodeY") temp_end_y = std::stoi(value_str);
                else if (key == "GenerationAlgorithms") algo_list_str = value_str;
            } catch (const std::exception& e) {
                std::cerr << "Warning: Invalid or out of range integer for key '" << key << "' in [MazeConfig]. Skipping. Error: " << e.what() << std::endl;
            }
        } else if (in_color_config_section) {
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
    ini_file.close();

    if (temp_start_x >= 0 && temp_start_x < MAZE_WIDTH && temp_start_y >= 0 && temp_start_y < MAZE_HEIGHT) {
        START_NODE = {temp_start_y, temp_start_x};
    } else {
        std::cerr << "Warning: Custom StartNode ("<< temp_start_y << "," << temp_start_x << ") from INI is out of bounds for maze dimensions ("
                  << MAZE_HEIGHT << "x" << MAZE_WIDTH << "). Using default (0,0)." << std::endl;
        START_NODE = {0,0};
    }
    
    int final_end_y = (temp_end_y != -1) ? temp_end_y : MAZE_HEIGHT - 1;
    int final_end_x = (temp_end_x != -1) ? temp_end_x : MAZE_WIDTH - 1;

    if (final_end_x >= 0 && final_end_x < MAZE_WIDTH && final_end_y >= 0 && final_end_y < MAZE_HEIGHT) {
        END_NODE = {final_end_y, final_end_x};
    } else {
        std::cerr << "Warning: Custom/Default EndNode (" << final_end_y << "," << final_end_x 
                  << ") is out of bounds for maze dimensions (" << MAZE_HEIGHT << "x" << MAZE_WIDTH 
                  << "). Adjusting to (" << MAZE_HEIGHT -1 << "," << MAZE_WIDTH -1 << ")." << std::endl;
        END_NODE = {MAZE_HEIGHT - 1, MAZE_WIDTH - 1};
    }
    // Ensure END_NODE is clamped to valid maze boundaries if MAZE_WIDTH/HEIGHT were small
    END_NODE.first = std::max(0, std::min(MAZE_HEIGHT - 1, END_NODE.first));
    END_NODE.second = std::max(0, std::min(MAZE_WIDTH - 1, END_NODE.second));


    if (MAZE_WIDTH > 0 && MAZE_HEIGHT > 0) { // Only check if maze is not degenerate
        if (START_NODE == END_NODE && (MAZE_WIDTH > 1 || MAZE_HEIGHT > 1)) { // Allow if 1x1 maze
             std::cerr << "Warning: START_NODE and END_NODE are identical (" << START_NODE.first << "," << START_NODE.second 
                       << "). This might lead to unexpected behavior for solvers." << std::endl;
        }
    }
    
    ACTIVE_GENERATION_ALGORITHMS.clear();
    if (!algo_list_str.empty()) {
        std::stringstream ss_algos(algo_list_str);
        std::string segment;
        while(std::getline(ss_algos, segment, ',')) {
            segment = trim(segment);
            std::transform(segment.begin(), segment.end(), segment.begin(), ::toupper); // Case-insensitive
            if (segment == "DFS") {
                ACTIVE_GENERATION_ALGORITHMS.push_back(MazeGeneration::MazeAlgorithmType::DFS);
            } else if (segment == "PRIMS") {
                ACTIVE_GENERATION_ALGORITHMS.push_back(MazeGeneration::MazeAlgorithmType::PRIMS);
            } else if (segment == "KRUSKAL") { // Added KRUSKAL
                ACTIVE_GENERATION_ALGORITHMS.push_back(MazeGeneration::MazeAlgorithmType::KRUSKAL);
            } else if (!segment.empty()) {
                std::cerr << "Warning: Unknown generation algorithm '" << segment << "' in config. Ignoring." << std::endl;
            }
        }
    }
    if (ACTIVE_GENERATION_ALGORITHMS.empty()) {
        std::cout << "Info: GenerationAlgorithms not specified or all invalid in config. Defaulting to DFS." << std::endl;
        ACTIVE_GENERATION_ALGORITHMS.push_back(MazeGeneration::MazeAlgorithmType::DFS);
    }


    std::cout << "Configuration successfully loaded." << std::endl;
    std::cout << "Maze Dimensions: " << MAZE_WIDTH << "x" << MAZE_HEIGHT << ", Unit Pixels: " << UNIT_PIXELS << std::endl;
    std::cout << "Start Node: (" << START_NODE.first << "," << START_NODE.second << "), End Node: (" << END_NODE.first << "," << END_NODE.second << ")" << std::endl;
    std::cout << "Selected Generation Algorithms: ";
    for(size_t i=0; i < ACTIVE_GENERATION_ALGORITHMS.size(); ++i) {
        switch(ACTIVE_GENERATION_ALGORITHMS[i]) {
            case MazeGeneration::MazeAlgorithmType::DFS: std::cout << "DFS"; break;
            case MazeGeneration::MazeAlgorithmType::PRIMS: std::cout << "PRIMS"; break;
            case MazeGeneration::MazeAlgorithmType::KRUSKAL: std::cout << "KRUSKAL"; break; // Added KRUSKAL
        }
        if (i < ACTIVE_GENERATION_ALGORITHMS.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
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
            const unsigned char* current_color_ptr = COLOR_WALL; 

            if (r_unit % 2 != 0 && c_unit % 2 != 0) { 
                int maze_r = (r_unit - 1) / 2;
                int maze_c = (c_unit - 1) / 2;
                current_color_ptr = COLOR_BACKGROUND; 

                SolverCellState state = visual_states[maze_r][maze_c];
                if (state == SolverCellState::START) current_color_ptr = COLOR_START;
                else if (state == SolverCellState::END) current_color_ptr = COLOR_END;
                else if (state == SolverCellState::SOLUTION) current_color_ptr = COLOR_SOLUTION_PATH;
                else if (state == SolverCellState::CURRENT_PROC) current_color_ptr = COLOR_CURRENT;
                else if (state == SolverCellState::FRONTIER) current_color_ptr = COLOR_FRONTIER;
                else if (state == SolverCellState::VISITED_PROC) current_color_ptr = COLOR_VISITED;

                if (state != SolverCellState::SOLUTION) { // Ensure start/end nodes keep their color if not part of solution yet
                     if (maze_r == START_NODE.first && maze_c == START_NODE.second && state != SolverCellState::END) current_color_ptr = COLOR_START;
                     if (maze_r == END_NODE.first && maze_c == END_NODE.second && state != SolverCellState::START) current_color_ptr = COLOR_END;
                }


            } else if (r_unit % 2 != 0 && c_unit % 2 == 0) { // Vertical Wall location
                int maze_r = (r_unit - 1) / 2;
                int maze_c_left = (c_unit / 2) - 1;
                if (c_unit > 0 && c_unit < 2 * MAZE_WIDTH) { 
                    if (maze_r >= 0 && maze_r < MAZE_HEIGHT && maze_c_left >= 0 && maze_c_left < MAZE_WIDTH) {
                       if (!maze[maze_r][maze_c_left].walls[1]) current_color_ptr = COLOR_BACKGROUND; // Check right wall of left cell
                    }
                }
            } else if (r_unit % 2 == 0 && c_unit % 2 != 0) { // Horizontal Wall location
                int maze_r_up = (r_unit / 2) - 1;
                int maze_c = (c_unit - 1) / 2;
                 if (r_unit > 0 && r_unit < 2 * MAZE_HEIGHT) { 
                    if (maze_r_up >= 0 && maze_r_up < MAZE_HEIGHT && maze_c >=0 && maze_c < MAZE_WIDTH) {
                        if (!maze[maze_r_up][maze_c].walls[2]) current_color_ptr = COLOR_BACKGROUND; // Check bottom wall of up cell
                    }
                }
            } // else it's a corner, always wall

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
    
    // Overlay solution path cells
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

    // Overlay solution path corridors
    if (current_path.size() > 1) {
        for (size_t i = 0; i < current_path.size() - 1; ++i) {
            std::pair<int, int> p1 = current_path[i];
            std::pair<int, int> p2 = current_path[i+1];

            int r1 = p1.first; int c1 = p1.second;
            int r2 = p2.first; int c2 = p2.second;

            int corridor_r_unit = -1, corridor_c_unit = -1;
            const unsigned char* corridor_color_ptr = COLOR_SOLUTION_PATH; // Could be different if needed

            if (r1 == r2) { // Horizontal corridor
                corridor_r_unit = 2 * r1 + 1;
                corridor_c_unit = 2 * std::min(c1, c2) + 2;
            } else if (c1 == c2) { // Vertical corridor
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
    if (p1.first == -1 || p1.second == -1 || // Invalid parent check
        p2.first == -1 || p2.second == -1 ||
        p3.first == -1 || p3.second == -1) {
        return false; 
    }
    // Check if they are collinear and p2 is between p1 and p3 (for this use case in solvers)
    int dr1 = p2.first - p1.first;
    int dc1 = p2.second - p1.second;
    int dr2 = p3.first - p2.first;
    int dc2 = p3.second - p2.second;
    return dr1 == dr2 && dc1 == dc2; // Simple check for same direction vector
}


// --- BFS Solver ---
void solve_bfs() {
    std::string folder = "bfs_frames_generated_by_" + CURRENT_GENERATION_ALGORITHM_NAME_FOR_FOLDER;
    fs::create_directories(folder); 
    std::cout << "Starting BFS for maze generated by " << CURRENT_GENERATION_ALGORITHM_NAME_FOR_FOLDER << " (frames in " << folder << ")..." << std::endl;

    if (MAZE_WIDTH <=0 || MAZE_HEIGHT <=0) {
        std::cout << "BFS: Invalid maze dimensions. Aborting." << std::endl;
        return;
    }
     if (START_NODE == END_NODE) {
        std::cout << "BFS: Start and End nodes are the same. Path is trivial." << std::endl;
        std::vector<std::vector<SolverCellState>> visual_states(MAZE_HEIGHT, std::vector<SolverCellState>(MAZE_WIDTH, SolverCellState::NONE));
        visual_states[START_NODE.first][START_NODE.second] = SolverCellState::SOLUTION;
        save_image(folder, 0, visual_states, {{START_NODE}});
        return;
    }

    std::vector<std::vector<SolverCellState>> visual_states(MAZE_HEIGHT, std::vector<SolverCellState>(MAZE_WIDTH, SolverCellState::NONE));
    std::vector<std::vector<bool>> visited_solver(MAZE_HEIGHT, std::vector<bool>(MAZE_WIDTH, false));
    std::queue<std::pair<int, int>> q;

    // Reset parents, as 'maze' global might have been used by another solver or algo
    for(int r=0; r<MAZE_HEIGHT; ++r) {
        for(int c=0; c<MAZE_WIDTH; ++c) {
            maze[r][c].parent = {-1, -1}; 
        }
    }

    q.push(START_NODE);
    visited_solver[START_NODE.first][START_NODE.second] = true;
    visual_states[START_NODE.first][START_NODE.second] = SolverCellState::FRONTIER; // Or START then FRONTIER
    int step = 0;
    save_image(folder, step++, visual_states); 

    std::vector<std::pair<int, int>> path;
    bool found = false;

    int dr[] = {-1, 1, 0, 0}; // Directions: Up, Down, Left, Right
    int dc[] = {0, 0, -1, 1}; 
    // Corresponding walls to check in current cell: maze[curr.first][curr.second].walls[i]
    // 0: Top, 1: Bottom, 2: Left, 3: Right (needs to match dr/dc to wall indices)
    // Wall indices for Cell struct: walls[0] (Top), walls[1] (Right), walls[2] (Bottom), walls[3] (Left)
    // dr/dc to wall_idx: Up (dr=-1) -> walls[0], Down (dr=1) -> walls[2], Left (dc=-1) -> walls[3], Right (dc=1) -> walls[1]
    int wall_check_indices[] = {0, 2, 3, 1}; // Order: Up, Down, Left, Right

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
        if (curr == END_NODE) { // Always save frame for reaching the end
            should_save_current_step_frames = true; 
        }


        visual_states[curr.first][curr.second] = SolverCellState::CURRENT_PROC;
        if (should_save_current_step_frames) {
            save_image(folder, step++, visual_states);
        }

        if (curr == END_NODE) {
            found = true;
        }

        if (!found) {
            for (int i = 0; i < 4; ++i) { // Iterate through Up, Down, Left, Right
                int nr = curr.first + dr[i];
                int nc = curr.second + dc[i];
                
                // Check if there's a wall
                bool wall_exists = maze[curr.first][curr.second].walls[wall_check_indices[i]];

                if (nr >= 0 && nr < MAZE_HEIGHT && nc >= 0 && nc < MAZE_WIDTH &&
                    !visited_solver[nr][nc] && !wall_exists) {
                    
                    visited_solver[nr][nc] = true;
                    maze[nr][nc].parent = curr;
                    q.push({nr, nc});
                    visual_states[nr][nc] = SolverCellState::FRONTIER;
                }
            }
        }
        
        if (curr != END_NODE || !found) { // Don't mark END as visited if it's the one being processed and found
             visual_states[curr.first][curr.second] = SolverCellState::VISITED_PROC; 
        }
       
        if (should_save_current_step_frames) {
            save_image(folder, step++, visual_states);
        }
        if (found && curr == END_NODE) break; 
    }

    if (found) {
        std::pair<int, int> curr_path_node = END_NODE;
        while (curr_path_node.first != -1 && curr_path_node.second != -1) { // Check for valid parent
            path.push_back(curr_path_node);
            visual_states[curr_path_node.first][curr_path_node.second] = SolverCellState::SOLUTION;
            if (curr_path_node == START_NODE) break; 
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
    std::string folder = "dfs_frames_generated_by_" + CURRENT_GENERATION_ALGORITHM_NAME_FOR_FOLDER;
    fs::create_directories(folder);
    std::cout << "Starting DFS for maze generated by " << CURRENT_GENERATION_ALGORITHM_NAME_FOR_FOLDER << " (frames in " << folder << ")..." << std::endl;


    if (MAZE_WIDTH <=0 || MAZE_HEIGHT <=0) {
        std::cout << "DFS: Invalid maze dimensions. Aborting." << std::endl;
        return;
    }
    if (START_NODE == END_NODE) {
        std::cout << "DFS: Start and End nodes are the same. Path is trivial." << std::endl;
        std::vector<std::vector<SolverCellState>> visual_states(MAZE_HEIGHT, std::vector<SolverCellState>(MAZE_WIDTH, SolverCellState::NONE));
        visual_states[START_NODE.first][START_NODE.second] = SolverCellState::SOLUTION;
        save_image(folder, 0, visual_states, {{START_NODE}});
        return;
    }

    std::vector<std::vector<SolverCellState>> visual_states(MAZE_HEIGHT, std::vector<SolverCellState>(MAZE_WIDTH, SolverCellState::NONE));
    std::vector<std::vector<bool>> visited_solver(MAZE_HEIGHT, std::vector<bool>(MAZE_WIDTH, false)); // For DFS, track cells whose children have all been explored
    std::stack<std::pair<int, int>> s;
    
    // Reset parents
    for(int r=0; r<MAZE_HEIGHT; ++r) {
        for(int c=0; c<MAZE_WIDTH; ++c) {
            maze[r][c].parent = {-1, -1}; 
        }
    }

    s.push(START_NODE);
    // For DFS, 'visited_solver' means cell is pushed onto stack.
    // A different 'fully_explored' might be needed if we want to show paths being backtracked from.
    // The current 'visited_solver' in BFS/DFS context usually means "added to frontier/stack".
    // Let's use visited_solver to mark cells that have been popped and processed.

    int step = 0;
    visual_states[START_NODE.first][START_NODE.second] = SolverCellState::FRONTIER; // Or CURRENT_PROC directly
    save_image(folder, step++, visual_states);


    std::vector<std::pair<int, int>> path;
    bool found = false;

    // Order of exploration: Up, Right, Down, Left (can be randomized for different DFS paths)
    int dr[] = {-1, 0, 1, 0}; 
    int dc[] = {0, 1, 0, -1};
    int wall_check_indices[] = {0, 1, 2, 3}; // Top, Right, Bottom, Left

    while (!s.empty() && !found) {
        std::pair<int, int> curr = s.top();
        // s.pop(); // Pop when fully explored, not when visiting

        bool should_save_frame_for_curr = true; // Default to saving
        if(visual_states[curr.first][curr.second] == SolverCellState::VISITED_PROC) { // Already processed this node and backtracked to it.
             s.pop(); // Now pop it as we are done with it and its children
             should_save_frame_for_curr = false; // No need to re-save image for a node we are just popping after backtrack
             continue;
        }


        if (!visited_solver[curr.first][curr.second]) { // First time visiting this node from stack top
             visited_solver[curr.first][curr.second] = true; // Mark as visited (conceptually, started processing)
             visual_states[curr.first][curr.second] = SolverCellState::CURRENT_PROC;

            std::pair<int, int> parent_cell = maze[curr.first][curr.second].parent;
            if (parent_cell.first != -1) { // Not the start node
                std::pair<int, int> grandparent_cell = maze[parent_cell.first][parent_cell.second].parent;
                if (grandparent_cell.first != -1) {
                    if (is_straight_line(grandparent_cell, parent_cell, curr)) {
                        should_save_frame_for_curr = false;
                    }
                }
            }
             if (curr == END_NODE) should_save_frame_for_curr = true;

            if (should_save_frame_for_curr) {
                save_image(folder, step++, visual_states);
            }
        }


        if (curr == END_NODE) {
            found = true;
            break; // Path found
        }

        bool pushed_neighbor = false;
        for (int i = 0; i < 4; ++i) { // Iterate directions
            int nr = curr.first + dr[i];
            int nc = curr.second + dc[i];
            
            bool wall_exists = maze[curr.first][curr.second].walls[wall_check_indices[i]];

            if (nr >= 0 && nr < MAZE_HEIGHT && nc >= 0 && nc < MAZE_WIDTH &&
                !visited_solver[nr][nc] && !wall_exists) { // If neighbor is valid, not visited, and no wall
                
                maze[nr][nc].parent = curr;
                s.push({nr, nc});
                visual_states[nr][nc] = SolverCellState::FRONTIER; // Mark as on stack (frontier)
                pushed_neighbor = true;
                // For a strict DFS, we'd break here and go deeper.
                // The current loop structure explores all neighbors before deepening from the last pushed one due to stack LIFO.
                // This is fine for finding *a* path.
                // If we want to show strict one-path-at-a-time exploration more clearly, we'd push one and recurse/continue loop.
                // Let's let it push all valid neighbors, then the stack LIFO will handle the depth-first nature.
            }
        }

        if (!pushed_neighbor) { // No unvisited children to explore from curr
            s.pop(); // Backtrack
            visual_states[curr.first][curr.second] = SolverCellState::VISITED_PROC; // Mark as fully explored
            if (should_save_frame_for_curr || curr == START_NODE) { // Save if it was a significant step or start node being exhausted
                 // Re-evaluate save necessity, as this is after exploring children
                bool re_save = true;
                std::pair<int, int> parent_cell = maze[curr.first][curr.second].parent;
                if(parent_cell.first != -1) {
                    std::pair<int, int> grandparent_cell = maze[parent_cell.first][parent_cell.second].parent;
                    if(grandparent_cell.first != -1 && is_straight_line(grandparent_cell, parent_cell, curr) && curr != END_NODE) {
                        re_save = false;
                    }
                }
                if(re_save) save_image(folder, step++, visual_states);
            }
        } else {
            // If we pushed a neighbor, the top of stack is now that neighbor.
            // The current 'curr' will be revisited later when stack unwinds to it,
            // at which point it might get popped if all its other children are explored.
            // To show 'curr' as 'visited' after pushing its first child to stack for processing:
            if (visual_states[curr.first][curr.second] == SolverCellState::CURRENT_PROC && should_save_frame_for_curr) {
                 // visual_states[curr.first][curr.second] = SolverCellState::VISITED_PROC; // Mark as visited (still on stack, but children being explored)
                 // save_image(folder, step++, visual_states); // This might create too many frames.
            }
        }
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
        save_image(folder, step++, visual_states, path); 
        std::cout << "DFS: Path found. Length: " << path.size() << std::endl;
    } else {
        std::cout << "DFS: Path not found." << std::endl;
        // Ensure the final state is saved if no path found
        save_image(folder, step++, visual_states); 
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
    if (ACTIVE_GENERATION_ALGORITHMS.empty()) {
        std::cerr << "Error: No maze generation algorithms selected after loading config. Exiting." << std::endl;
        return 1;
    }
    
    std::vector<std::vector<MazeGeneration::GenCell>> generation_maze_data;

    for (const auto& algo_type : ACTIVE_GENERATION_ALGORITHMS) {
        switch (algo_type) {
            case MazeGeneration::MazeAlgorithmType::DFS:
                CURRENT_GENERATION_ALGORITHM_NAME_FOR_FOLDER = "DFS";
                break;
            case MazeGeneration::MazeAlgorithmType::PRIMS:
                CURRENT_GENERATION_ALGORITHM_NAME_FOR_FOLDER = "Prims";
                break;
            case MazeGeneration::MazeAlgorithmType::KRUSKAL: // Added KRUSKAL
                CURRENT_GENERATION_ALGORITHM_NAME_FOR_FOLDER = "Kruskal";
                break;
            default: // Should not happen if config loading is correct
                CURRENT_GENERATION_ALGORITHM_NAME_FOR_FOLDER = "UnknownAlgo";
                break;
        }
        std::cout << "\n--- Processing for Maze Generation Algorithm: " 
                  << CURRENT_GENERATION_ALGORITHM_NAME_FOR_FOLDER << " ---" << std::endl;

        // Prepare/reset the generation-specific maze structure for the current algorithm
        // assign ensures fresh GenCell objects (walls up, unvisited)
        generation_maze_data.assign(MAZE_HEIGHT, std::vector<MazeGeneration::GenCell>(MAZE_WIDTH));

        std::cout << "Generating maze structure..." << std::endl;
        // Use START_NODE from config as a hint for generation start, clamped to bounds
        // For Kruskal, start_node is not directly used in its logic, but passed for consistency.
        int gen_start_r = (START_NODE.first >= 0 && START_NODE.first < MAZE_HEIGHT) ? START_NODE.first : 0;
        int gen_start_c = (START_NODE.second >= 0 && START_NODE.second < MAZE_WIDTH) ? START_NODE.second : 0;
        
        if (!(START_NODE.first >= 0 && START_NODE.first < MAZE_HEIGHT && 
              START_NODE.second >= 0 && START_NODE.second < MAZE_WIDTH)) {
             if (algo_type == MazeGeneration::MazeAlgorithmType::DFS || algo_type == MazeGeneration::MazeAlgorithmType::PRIMS) {
                std::cout << "Adjusted maze generation start point to (" << gen_start_r << "," << gen_start_c 
                          << ") due to out-of-bounds config START_NODE for DFS/Prims." << std::endl;
             }
        }

        MazeGeneration::generate_maze_structure(generation_maze_data, gen_start_r, gen_start_c, MAZE_WIDTH, MAZE_HEIGHT, algo_type);

        // Transfer wall data from generation_maze_data to the main 'maze' variable for solvers
        maze.assign(MAZE_HEIGHT, std::vector<Cell>(MAZE_WIDTH)); // Resets .parent for solver
        for (int r = 0; r < MAZE_HEIGHT; ++r) {
            for (int c = 0; c < MAZE_WIDTH; ++c) {
                for (int i = 0; i < 4; ++i) {
                    maze[r][c].walls[i] = generation_maze_data[r][c].walls[i];
                }
            }
        }
        std::cout << "Maze generated and data transferred." << std::endl;

        solve_bfs();
        // No need to reset maze[r][c].parent here, as solve_bfs/dfs do it internally.
        solve_dfs();
    }

    std::cout << "\nProcessing complete for all selected algorithms. Images saved in respective folders." << std::endl;

    return 0;
}