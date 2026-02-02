#ifndef MAZE_GENERATION_H
#define MAZE_GENERATION_H

#include <string>
#include <string_view>
#include <vector>

#include "domain/maze_generation.h"

namespace MazeGeneration {

using MazeAlgorithmType = MazeDomain::MazeAlgorithmType;
using MazeGrid = MazeDomain::MazeGrid;

// Application-layer wrapper: keeps the existing API while delegating to the
// domain.
void generate_maze_structure(MazeGrid& maze_grid_to_populate, int start_r,
                             int start_c, int grid_width, int grid_height,
                             MazeAlgorithmType algorithm_type);

std::string algorithm_name(MazeAlgorithmType algorithm_type);
bool try_parse_algorithm(std::string_view name, MazeAlgorithmType& out_type);
std::vector<std::string> supported_algorithms();

}  // namespace MazeGeneration

#endif  // MAZE_GENERATION_H
