#include "config_loader.h"
#include "toml++/toml.hpp" // 引入 toml++ 库

#include <iostream>
#include <algorithm>
#include <string_view>

namespace ConfigLoader {

// --- Configuration Variables Definitions ---
int MAZE_WIDTH = 10;
int MAZE_HEIGHT = 10;
int UNIT_PIXELS = 15;
std::pair<int, int> START_NODE = {0, 0};
std::pair<int, int> END_NODE = {9, 9}; // Initialize with a default
std::vector<AlgorithmInfo> ACTIVE_GENERATION_ALGORITHMS;
std::string CURRENT_GENERATION_ALGORITHM_NAME_FOR_FOLDER;

// --- Color Definitions (R, G, B) ---
unsigned char COLOR_BACKGROUND[3] = {255, 255, 255};
unsigned char COLOR_WALL[3] = {0, 0, 0};
unsigned char COLOR_OUTER_WALL[3] = {74, 74, 74};
unsigned char COLOR_INNER_WALL[3] = {0, 0, 0};
unsigned char COLOR_START[3] = {0, 255, 0};
unsigned char COLOR_END[3] = {255, 0, 0};
unsigned char COLOR_FRONTIER[3] = {173, 216, 230};
unsigned char COLOR_VISITED[3] = {211, 211, 211};
unsigned char COLOR_CURRENT[3] = {255, 165, 0};
unsigned char COLOR_SOLUTION_PATH[3] = {0, 0, 255};

// Helper to trim whitespace from the start of a string_view
std::string_view trim_left(std::string_view s) {
    s.remove_prefix(std::min(s.find_first_not_of(" \t\n\r\f\v"), s.size()));
    return s;
}

bool parse_hex_color_string(std::string hex_str, unsigned char& r, unsigned char& g, unsigned char& b) {
    // This function can be simplified as toml++ gives us clean strings
    if (hex_str.empty()) return false;
    if (hex_str[0] == '#') {
        hex_str = hex_str.substr(1);
    }
    if (hex_str.length() != 6) {
        std::cerr << "Warning: Hex color string '#" << hex_str << "' must be 6 characters long.\n";
        return false;
    }
    try {
        r = static_cast<unsigned char>(std::stoul(hex_str.substr(0, 2), nullptr, 16));
        g = static_cast<unsigned char>(std::stoul(hex_str.substr(2, 2), nullptr, 16));
        b = static_cast<unsigned char>(std::stoul(hex_str.substr(4, 2), nullptr, 16));
        return true;
    } catch (const std::exception&) {
        std::cerr << "Warning: Invalid character in hex color string '#" << hex_str << "'.\n";
        return false;
    }
}


void load_config(const std::string& filename) {
    toml::table config;
    try {
        config = toml::parse_file(filename);
    } catch (const toml::parse_error& err) {
        std::cerr << "Error parsing config file '" << filename << "':\n" << err << "\nUsing default values.\n";
        // Setup default values if file is missing or broken
        MAZE_WIDTH = 10;
        MAZE_HEIGHT = 10;
        UNIT_PIXELS = 15;
        START_NODE = {0, 0};
        END_NODE = {MAZE_HEIGHT - 1, MAZE_WIDTH - 1};
        ACTIVE_GENERATION_ALGORITHMS.push_back({MazeGeneration::MazeAlgorithmType::DFS, "DFS"});
        return;
    }

    // --- [MazeConfig] ---
    MAZE_WIDTH = config["MazeConfig"]["MazeWidth"].value_or(10);
    MAZE_HEIGHT = config["MazeConfig"]["MazeHeight"].value_or(10);
    UNIT_PIXELS = config["MazeConfig"]["UnitPixels"].value_or(15);

    int start_y = config["MazeConfig"]["StartNodeY"].value_or(0);
    int start_x = config["MazeConfig"]["StartNodeX"].value_or(0);
    START_NODE = {start_y, start_x};

    // Use value_or to provide a default if the key doesn't exist
    int end_y = config["MazeConfig"]["EndNodeY"].value_or(MAZE_HEIGHT - 1);
    int end_x = config["MazeConfig"]["EndNodeX"].value_or(MAZE_WIDTH - 1);
    END_NODE = {end_y, end_x};
    
    // --- Parse algorithms array ---
    ACTIVE_GENERATION_ALGORITHMS.clear();
    if (auto algos = config["MazeConfig"]["GenerationAlgorithms"].as_array()) {
        for (const auto& elem : *algos) {
            if (auto algo_name = elem.value<std::string>()) {
                std::string upper_name = *algo_name;
                std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(), ::toupper);

                if (upper_name == "DFS") {
                    ACTIVE_GENERATION_ALGORITHMS.push_back({MazeGeneration::MazeAlgorithmType::DFS, "DFS"});
                } else if (upper_name == "PRIMS") {
                    ACTIVE_GENERATION_ALGORITHMS.push_back({MazeGeneration::MazeAlgorithmType::PRIMS, "Prims"});
                } else if (upper_name == "KRUSKAL") {
                    ACTIVE_GENERATION_ALGORITHMS.push_back({MazeGeneration::MazeAlgorithmType::KRUSKAL, "Kruskal"});
                } else {
                    std::cerr << "Warning: Unknown generation algorithm '" << *algo_name << "' in config. Ignoring.\n";
                }
            }
        }
    }
    if (ACTIVE_GENERATION_ALGORITHMS.empty()) {
        std::cout << "Info: No valid generation algorithms specified. Defaulting to DFS.\n";
        ACTIVE_GENERATION_ALGORITHMS.push_back({MazeGeneration::MazeAlgorithmType::DFS, "DFS"});
    }

    // --- [ColorConfig] ---
    // Helper lambda to load a color
    auto load_color = [&](const char* key, unsigned char* color_arr) {
        if (auto color_str = config["ColorConfig"][key].value<std::string>()) {
            parse_hex_color_string(*color_str, color_arr[0], color_arr[1], color_arr[2]);
        }
    };
    
    load_color("BackgroundColor", COLOR_BACKGROUND);
    load_color("OuterWallColor", COLOR_OUTER_WALL);
    load_color("InnerWallColor", COLOR_INNER_WALL);
    load_color("StartNodeColor", COLOR_START);
    load_color("EndNodeColor", COLOR_END);
    load_color("FrontierColor", COLOR_FRONTIER);
    load_color("VisitedColor", COLOR_VISITED);
    load_color("CurrentProcessingColor", COLOR_CURRENT);
    load_color("SolutionPathColor", COLOR_SOLUTION_PATH);

    // --- Final validation and printout ---
    std::cout << "Configuration successfully loaded from " << filename << std::endl;
    std::cout << "Maze Dimensions: " << MAZE_WIDTH << "x" << MAZE_HEIGHT << ", Unit Pixels: " << UNIT_PIXELS << std::endl;
    std::cout << "Start Node: (" << START_NODE.first << "," << START_NODE.second << "), End Node: (" << END_NODE.first << "," << END_NODE.second << ")" << std::endl;
    std::cout << "Selected Generation Algorithms: ";
    for(size_t i=0; i < ACTIVE_GENERATION_ALGORITHMS.size(); ++i) {
        std::cout << ACTIVE_GENERATION_ALGORITHMS[i].name;
        if (i < ACTIVE_GENERATION_ALGORITHMS.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
}

} // namespace ConfigLoader