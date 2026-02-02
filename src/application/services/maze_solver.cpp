#include "application/services/maze_solver.h"

#include <iostream>

namespace {

using GridPosition = MazeSolverDomain::GridPosition;

auto IsValidPosition(GridPosition pos, int height, int width) -> bool {
  return pos.first >= 0 && pos.first < height && pos.second >= 0 &&
         pos.second < width;
}

}  // namespace

namespace MazeSolver {

auto AlgorithmName(SolverAlgorithmType algorithm_type) -> std::string {
  return MazeSolverDomain::AlgorithmName(algorithm_type);
}

auto TryParseAlgorithm(std::string_view name, SolverAlgorithmType& out_type)
    -> bool {
  return MazeSolverDomain::TryParseAlgorithm(name, out_type);
}

auto Solve(const MazeDomain::MazeGrid& maze_data,
           SolverAlgorithmType algorithm_type, const Config::AppConfig& config)
    -> SearchResult {
  const auto& maze = config.maze;
  const std::string kSolverName =
      MazeSolverDomain::AlgorithmName(algorithm_type);
  const std::string kDisplayName = kSolverName.empty() ? "Solver" : kSolverName;

  if (maze.width <= 0 || maze.height <= 0) {
    std::cout << kDisplayName << ": Invalid maze dimensions. Aborting."
              << std::endl;
    return {};
  }

  const auto kHeightSize = static_cast<size_t>(maze.height);
  const auto kWidthSize = static_cast<size_t>(maze.width);
  if (maze_data.size() != kHeightSize) {
    std::cerr << kDisplayName
              << ": Maze grid height does not match config. Aborting."
              << std::endl;
    return {};
  }
  if (!maze_data.empty() && maze_data.front().size() != kWidthSize) {
    std::cerr << kDisplayName
              << ": Maze grid width does not match config. Aborting."
              << std::endl;
    return {};
  }

  if (!IsValidPosition(maze.start_node, maze.height, maze.width)) {
    std::cerr << kDisplayName << ": Start node is out of bounds. Aborting."
              << std::endl;
    return {};
  }
  if (!IsValidPosition(maze.end_node, maze.height, maze.width)) {
    std::cerr << kDisplayName << ": End node is out of bounds. Aborting."
              << std::endl;
    return {};
  }

  if (maze.start_node == maze.end_node) {
    std::cout << kDisplayName
              << ": Start and End nodes are the same. Path is trivial."
              << std::endl;
  }

  SearchResult result = MazeSolverDomain::Solve(maze_data, maze.start_node,
                                                maze.end_node, algorithm_type);

  if (result.found_) {
    std::cout << kDisplayName << ": Path found. Length: " << result.path_.size()
              << std::endl;
  } else {
    std::cout << kDisplayName << ": Path not found." << std::endl;
  }

  return result;
}

}  // namespace MazeSolver
