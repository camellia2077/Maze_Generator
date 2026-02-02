#ifndef MAZE_RENDERER_H
#define MAZE_RENDERER_H

#include <cstddef>
#include <string>
#include <string_view>

#include "config/config.h"
#include "domain/maze_grid.h"
#include "domain/maze_solver.h"

namespace MazeSolver {

struct RenderResult {
  bool ok = true;
  std::string error;
  std::string output_folder;
  size_t frames_written = 0;
};

RenderResult RenderSearchResult(
    const MazeSolverDomain::SearchResult& result,
    const MazeDomain::MazeGrid& maze_ref,
    MazeSolverDomain::SolverAlgorithmType algorithm_type,
    std::string_view generation_algorithm_name,
    const Config::AppConfig& config);

}  // namespace MazeSolver

#endif  // MAZE_RENDERER_H
