#ifndef MAZE_DOMAIN_MAZE_SOLVER_ALGORITHMS_H
#define MAZE_DOMAIN_MAZE_SOLVER_ALGORITHMS_H

#include "domain/maze_solver_common.h"

namespace MazeSolverDomain::detail {

auto SolveBfs(const MazeGrid& maze_grid, GridPosition start_node,
              GridPosition end_node) -> SearchResult;
auto SolveDfs(const MazeGrid& maze_grid, GridPosition start_node,
              GridPosition end_node) -> SearchResult;
auto SolveAStar(const MazeGrid& maze_grid, GridPosition start_node,
                GridPosition end_node) -> SearchResult;
auto SolveDijkstra(const MazeGrid& maze_grid, GridPosition start_node,
                   GridPosition end_node) -> SearchResult;
auto SolveGreedyBestFirst(const MazeGrid& maze_grid, GridPosition start_node,
                          GridPosition end_node) -> SearchResult;

}  // namespace MazeSolverDomain::detail

#endif  // MAZE_DOMAIN_MAZE_SOLVER_ALGORITHMS_H
