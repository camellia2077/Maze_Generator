#include "config_loader.h"

#include <fstream>
#include <sstream>
#include <algorithm> // For std::max and std::transform
#include <iostream>  // For std::cerr, std::cout
#include <vector>

namespace ConfigLoader {

// --- Configuration Variables Definitions ---
int MAZE_WIDTH = 10;    
int MAZE_HEIGHT = 10;   
int UNIT_PIXELS = 15;   
std::pair<int, int> START_NODE = {0, 0};
std::pair<int, int> END_NODE = {0, 0}; // Will be properly set in load_config
// std::vector<MazeGeneration::MazeAlgorithmType> ACTIVE_GENERATION_ALGORITHMS; // Old
std::vector<AlgorithmInfo> ACTIVE_GENERATION_ALGORITHMS; // New: Using AlgorithmInfo
std::string CURRENT_GENERATION_ALGORITHM_NAME_FOR_FOLDER;


// --- Color Definitions (R, G, B) ---
unsigned char COLOR_BACKGROUND[3] = {255, 255, 255};
unsigned char COLOR_WALL[3] = {0, 0, 0}; // Default if specific wall colors are not loaded
unsigned char COLOR_OUTER_WALL[3] = {74, 74, 74};    // Default Dark Gray for outer walls
unsigned char COLOR_INNER_WALL[3] = {0, 0, 0};       // Default Black for inner walls
unsigned char COLOR_START[3] = {0, 255, 0};
unsigned char COLOR_END[3] = {255, 0, 0};
unsigned char COLOR_FRONTIER[3] = {173, 216, 230};
unsigned char COLOR_VISITED[3] = {211, 211, 211};
unsigned char COLOR_CURRENT[3] = {255, 165, 0};
unsigned char COLOR_SOLUTION_PATH[3] = {0, 0, 255};


std::string trim(const std::string& str) {
    const std::string whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) return ""; 
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

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

void load_config(const std::string& filename) {
    MAZE_WIDTH = 10;
    MAZE_HEIGHT = 10;
    UNIT_PIXELS = 15;
    START_NODE = {0,0};

    std::ifstream ini_file(filename);
    if (!ini_file.is_open()) {
        std::cerr << "Warning: Could not open config file '" << filename << "'. Using default values for all settings." << std::endl;
        END_NODE = {MAZE_HEIGHT - 1, MAZE_WIDTH - 1};
        ACTIVE_GENERATION_ALGORITHMS.clear();
        ACTIVE_GENERATION_ALGORITHMS.push_back({MazeGeneration::MazeAlgorithmType::DFS, "DFS"}); // Default algo
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
                else if (key == "WallColor") { COLOR_WALL[0]=r; COLOR_WALL[1]=g; COLOR_WALL[2]=b; } // Keep for backward compatibility or as a general fallback if others are not set
                else if (key == "OuterWallColor") { COLOR_OUTER_WALL[0]=r; COLOR_OUTER_WALL[1]=g; COLOR_OUTER_WALL[2]=b; }
                else if (key == "InnerWallColor") { COLOR_INNER_WALL[0]=r; COLOR_INNER_WALL[1]=g; COLOR_INNER_WALL[2]=b; }
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
    // Logic for START_NODE, END_NODE, and ACTIVE_GENERATION_ALGORITHMS remains the same
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
    END_NODE.first = std::max(0, std::min(MAZE_HEIGHT - 1, END_NODE.first));
    END_NODE.second = std::max(0, std::min(MAZE_WIDTH - 1, END_NODE.second));

    if (MAZE_WIDTH > 0 && MAZE_HEIGHT > 0) {
        if (START_NODE == END_NODE && (MAZE_WIDTH > 1 || MAZE_HEIGHT > 1)) {
             std::cerr << "Warning: START_NODE and END_NODE are identical (" << START_NODE.first << "," << START_NODE.second 
                       << "). This might lead to unexpected behavior for solvers." << std::endl;
        }
    }
    
    ACTIVE_GENERATION_ALGORITHMS.clear();
    if (!algo_list_str.empty()) {
        std::stringstream ss_algos(algo_list_str);
        std::string segment;
        std::string original_segment; // Store original segment for name
        while(std::getline(ss_algos, segment, ',')) {
            original_segment = trim(segment); // Trim original for a clean name
            std::string upper_segment = original_segment;
            std::transform(upper_segment.begin(), upper_segment.end(), upper_segment.begin(), ::toupper);
            
            if (upper_segment == "DFS") {
                ACTIVE_GENERATION_ALGORITHMS.push_back({MazeGeneration::MazeAlgorithmType::DFS, "DFS"});
            } else if (upper_segment == "PRIMS") {
                ACTIVE_GENERATION_ALGORITHMS.push_back({MazeGeneration::MazeAlgorithmType::PRIMS, "Prims"});
            } else if (upper_segment == "KRUSKAL") {
                ACTIVE_GENERATION_ALGORITHMS.push_back({MazeGeneration::MazeAlgorithmType::KRUSKAL, "Kruskal"});
            } else if (!original_segment.empty()) { // Check original_segment to avoid warning for empty strings from "DFS,"
                std::cerr << "Warning: Unknown generation algorithm '" << original_segment << "' in config. Ignoring." << std::endl;
            }
        }
    }
    if (ACTIVE_GENERATION_ALGORITHMS.empty()) {
        std::cout << "Info: GenerationAlgorithms not specified or all invalid in config. Defaulting to DFS." << std::endl;
        ACTIVE_GENERATION_ALGORITHMS.push_back({MazeGeneration::MazeAlgorithmType::DFS, "DFS"});
    }

    std::cout << "Configuration successfully loaded." << std::endl;
    std::cout << "Maze Dimensions: " << MAZE_WIDTH << "x" << MAZE_HEIGHT << ", Unit Pixels: " << UNIT_PIXELS << std::endl;
    std::cout << "Start Node: (" << START_NODE.first << "," << START_NODE.second << "), End Node: (" << END_NODE.first << "," << END_NODE.second << ")" << std::endl;
    std::cout << "Selected Generation Algorithms: ";
    for(size_t i=0; i < ACTIVE_GENERATION_ALGORITHMS.size(); ++i) {
        std::cout << ACTIVE_GENERATION_ALGORITHMS[i].name; // Use AlgorithmInfo.name
        if (i < ACTIVE_GENERATION_ALGORITHMS.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
}

} // namespace ConfigLoader