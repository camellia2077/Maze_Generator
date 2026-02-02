#include "domain/maze_solver_algorithms.h"

#include <array>
#include <limits>
#include <queue>

namespace MazeSolverDomain::detail {

namespace {

void EnqueueDijkstraNeighbors(
    const MazeGrid& maze_grid, GridSize grid_size, GridPosition current,
    const DirectionDeltas& deltas, const BoolGrid& visited, IntGrid& g_scores,
    ParentGrid& parents,
    std::priority_queue<AStarNode, std::vector<AStarNode>, AStarNodeCompare>&
        frontier,
    StateGrid& visual_states) {
  for (int dir_index = 0; dir_index < kWallCount; ++dir_index) {
    const int kNextR = current.first + deltas.row_delta[dir_index];
    const int kNextC = current.second + deltas.col_delta[dir_index];

    const bool kWallExists = maze_grid[current.first][current.second]
                                 .walls[deltas.wall_check_index[dir_index]];

    if (kNextR >= 0 && kNextR < grid_size.height && kNextC >= 0 &&
        kNextC < grid_size.width && !visited[kNextR][kNextC] && !kWallExists) {
      const int kTentativeG = g_scores[current.first][current.second] + 1;
      if (kTentativeG < g_scores[kNextR][kNextC]) {
        g_scores[kNextR][kNextC] = kTentativeG;
        parents[kNextR][kNextC] = current;
        frontier.push({kTentativeG, kTentativeG, {kNextR, kNextC}});
        visual_states[kNextR][kNextC] = SolverCellState::FRONTIER;
      }
    }
  }
}

}  // namespace

auto SolveDijkstra(const MazeGrid& maze_grid, GridPosition start_node,
                   GridPosition end_node) -> SearchResult {
  SearchResult result;
  const auto kGridSize = GetGridSize(maze_grid);
  if (!kGridSize.has_value()) {
    return result;
  }
  if (!IsValidPosition(start_node, *kGridSize) ||
      !IsValidPosition(end_node, *kGridSize)) {
    return result;
  }

  if (start_node == end_node) {
    return CreateTrivialResult(*kGridSize, start_node);
  }

  const PathEndpoints kEndpoints{.start = start_node, .end = end_node};
  auto visual_states = CreateStateGrid(*kGridSize, SolverCellState::NONE);
  auto visited = CreateBoolGrid(*kGridSize, false);
  auto parents = CreateParentGrid(*kGridSize, kInvalidCell);

  const int kMaxCost = std::numeric_limits<int>::max() / kMaxCostDivisor;
  auto g_scores = CreateIntGrid(*kGridSize, kMaxCost);
  std::priority_queue<AStarNode, std::vector<AStarNode>, AStarNodeCompare>
      frontier;

  g_scores[start_node.first][start_node.second] = 0;
  frontier.push({0, 0, start_node});
  visual_states[start_node.first][start_node.second] =
      SolverCellState::FRONTIER;
  PushFrame(result, visual_states, {});

  constexpr std::array<int, kWallCount> kRowDelta = {-1, 1, 0, 0};
  constexpr std::array<int, kWallCount> kColDelta = {0, 0, -1, 1};
  constexpr std::array<int, kWallCount> kWallCheckIndex = {
      kWallTop, kWallBottom, kWallLeft, kWallRight};
  const DirectionDeltas kDeltas{.row_delta = kRowDelta,
                                .col_delta = kColDelta,
                                .wall_check_index = kWallCheckIndex};

  bool found = false;
  while (!frontier.empty() && !found) {
    const AStarNode kCurrentNode = frontier.top();
    frontier.pop();

    const GridPosition kCurrent = kCurrentNode.pos;
    if (visited[kCurrent.first][kCurrent.second]) {
      continue;
    }

    const bool kShouldSaveFrame =
        ShouldSaveFrameForCurrent(parents, kCurrent, kEndpoints);

    visited[kCurrent.first][kCurrent.second] = true;
    visual_states[kCurrent.first][kCurrent.second] =
        SolverCellState::CURRENT_PROC;
    if (kShouldSaveFrame) {
      PushFrame(result, visual_states, {});
    }

    if (kCurrent == end_node) {
      found = true;
      break;
    }

    EnqueueDijkstraNeighbors(maze_grid, *kGridSize, kCurrent, kDeltas, visited,
                             g_scores, parents, frontier, visual_states);

    if (kCurrent != end_node || !found) {
      visual_states[kCurrent.first][kCurrent.second] =
          SolverCellState::VISITED_PROC;
    }

    if (kShouldSaveFrame) {
      PushFrame(result, visual_states, {});
    }
  }

  FinalizeSearchResult(found, kEndpoints, parents, visual_states,
                       std::move(visited), result);
  return result;
}

}  // namespace MazeSolverDomain::detail
