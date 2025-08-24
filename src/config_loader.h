#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <string>
#include <vector>
#include <utility> // For std::pair
#include "maze_generation.h" // For MazeGeneration::MazeAlgorithmType

namespace ConfigLoader {

// Structure to hold algorithm type and its string name
struct AlgorithmInfo {
    MazeGeneration::MazeAlgorithmType type;
    std::string name; // User-friendly name, e.g., "DFS", "Prims", "Kruskal"
};

// --- Configuration Variables ---
extern int MAZE_WIDTH;
extern int MAZE_HEIGHT;
extern int UNIT_PIXELS;
extern std::pair<int, int> START_NODE;
extern std::pair<int, int> END_NODE;
// extern std::vector<MazeGeneration::MazeAlgorithmType> ACTIVE_GENERATION_ALGORITHMS; // Old
extern std::vector<AlgorithmInfo> ACTIVE_GENERATION_ALGORITHMS; // New: Using AlgorithmInfo
extern std::string CURRENT_GENERATION_ALGORITHM_NAME_FOR_FOLDER;

// --- Color Definitions (R, G, B) ---
extern unsigned char COLOR_BACKGROUND[3];
extern unsigned char COLOR_WALL[3]; // Remains for now, or could be removed if fully replaced
extern unsigned char COLOR_OUTER_WALL[3]; // New
extern unsigned char COLOR_INNER_WALL[3]; // New
extern unsigned char COLOR_START[3];
extern unsigned char COLOR_END[3];
extern unsigned char COLOR_FRONTIER[3];
extern unsigned char COLOR_VISITED[3];
extern unsigned char COLOR_CURRENT[3];
extern unsigned char COLOR_SOLUTION_PATH[3];

// --- Utility function to trim whitespace ---
std::string trim(const std::string& str);

// --- Utility function to parse Hex color string ---
bool parse_hex_color_string(std::string hex_str, unsigned char& r, unsigned char& g, unsigned char& b);

// --- Function to load configuration from INI file ---
void load_config(const std::string& filename);

} // namespace ConfigLoader

#endif // CONFIG_LOADER_H