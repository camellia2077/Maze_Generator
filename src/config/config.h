#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <utility>
#include <vector>

#include "application/services/maze_generation.h"
#include "domain/maze_solver.h"

namespace Config {

struct AlgorithmInfo {
  MazeGeneration::MazeAlgorithmType type;
  std::string name;
};

struct SearchAlgorithmInfo {
  MazeSolverDomain::SolverAlgorithmType type;
  std::string name;
};

struct MazeConfig {
  int width = 10;
  int height = 10;
  int unit_pixels = 15;
  std::pair<int, int> start_node = {0, 0};
  std::pair<int, int> end_node = {0, 0};
  std::vector<AlgorithmInfo> generation_algorithms;
  std::vector<SearchAlgorithmInfo> search_algorithms;
};

struct ColorConfig {
  unsigned char background[3] = {255, 255, 255};
  unsigned char wall[3] = {0, 0, 0};
  unsigned char outer_wall[3] = {74, 74, 74};
  unsigned char inner_wall[3] = {0, 0, 0};
  unsigned char start[3] = {0, 255, 0};
  unsigned char end[3] = {255, 0, 0};
  unsigned char frontier[3] = {173, 216, 230};
  unsigned char visited[3] = {211, 211, 211};
  unsigned char current[3] = {255, 165, 0};
  unsigned char solution_path[3] = {0, 0, 255};
};

struct AppConfig {
  MazeConfig maze;
  ColorConfig colors;
  std::string output_dir = ".";
};

}  // namespace Config

#endif  // CONFIG_H
