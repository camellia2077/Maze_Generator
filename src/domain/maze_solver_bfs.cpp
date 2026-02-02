#include "domain/maze_solver_algorithms.h"

#include <array>
#include <queue>

namespace MazeSolverDomain::detail {

namespace {

void EnqueueBfsNeighbors(const MazeGrid& maze_grid, GridSize grid_size,
                         GridPosition current,
                         const DirectionDeltas& deltas,
                         BoolGrid& visited, ParentGrid& parents,
                         std::queue<GridPosition>& frontier,
                         StateGrid& visual_states) {
  for (int dir_index = 0; dir_index < kWallCount; ++dir_index) {
    const int kNextR = current.first + deltas.row_delta[dir_index];
    const int kNextC = current.second + deltas.col_delta[dir_index];

    const bool kWallExists = maze_grid[current.first][current.second]
                                 .walls[deltas.wall_check_index[dir_index]];

    if (kNextR >= 0 && kNextR < grid_size.height && kNextC >= 0 &&
        kNextC < grid_size.width && !visited[kNextR][kNextC] &&
        !kWallExists) {
      visited[kNextR][kNextC] = true;
      parents[kNextR][kNextC] = current;
      frontier.emplace(kNextR, kNextC);
      visual_states[kNextR][kNextC] = SolverCellState::FRONTIER;
    }
  }
}

}  // namespace

auto SolveBfs(const MazeGrid& maze_grid, GridPosition start_node,
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

  std::queue<GridPosition> frontier;
  frontier.push(start_node);
  visited[start_node.first][start_node.second] = true;
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
    const GridPosition kCurrent = frontier.front();
    frontier.pop();

    const bool kShouldSaveFrame =
        ShouldSaveFrameForCurrent(parents, kCurrent, kEndpoints);

    visual_states[kCurrent.first][kCurrent.second] =
        SolverCellState::CURRENT_PROC;
    if (kShouldSaveFrame) {
      PushFrame(result, visual_states, {});
    }

    if (kCurrent == end_node) {
      found = true;
    }

    if (!found) {
      EnqueueBfsNeighbors(maze_grid, *kGridSize, kCurrent, kDeltas, visited,
                          parents, frontier, visual_states);
    }

    if (kCurrent != end_node || !found) {
      visual_states[kCurrent.first][kCurrent.second] =
          SolverCellState::VISITED_PROC;
    }

    if (kShouldSaveFrame) {
      PushFrame(result, visual_states, {});
    }
    if (found && kCurrent == end_node) {
      break;
    }
  }

  FinalizeSearchResult(found, kEndpoints, parents, visual_states,
                       std::move(visited), result);
  return result;
}

}  // namespace MazeSolverDomain::detail
