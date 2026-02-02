#include "domain/maze_solver_common.h"

#include <algorithm>
#include <cstdlib>

namespace MazeSolverDomain::detail {

namespace {

struct StraightLineTriplet {
  GridPosition first;
  GridPosition second;
  GridPosition third;
};

auto IsStraightLine(StraightLineTriplet line) -> bool {
  if (line.first == kInvalidCell || line.second == kInvalidCell ||
      line.third == kInvalidCell) {
    return false;
  }
  const int kDeltaR1 = line.second.first - line.first.first;
  const int kDeltaC1 = line.second.second - line.first.second;
  const int kDeltaR2 = line.third.first - line.second.first;
  const int kDeltaC2 = line.third.second - line.second.second;
  return kDeltaR1 == kDeltaR2 && kDeltaC1 == kDeltaC2;
}

auto ShouldSkipStraightLineFrame(const ParentGrid& parents,
                                 GridPosition current) -> bool {
  const GridPosition kParentCell = parents[current.first][current.second];
  if (kParentCell == kInvalidCell) {
    return false;
  }
  const GridPosition kGrandparentCell =
      parents[kParentCell.first][kParentCell.second];
  if (kGrandparentCell == kInvalidCell) {
    return false;
  }
  const StraightLineTriplet kLine{
      .first = kGrandparentCell, .second = kParentCell, .third = current};
  return IsStraightLine(kLine);
}

void AppendSolutionPath(const PathEndpoints& endpoints,
                        const ParentGrid& parents,
                        StateGrid& visual_states, SearchResult& result) {
  std::vector<GridPosition> path;
  GridPosition path_node = endpoints.end;
  while (path_node != kInvalidCell) {
    path.push_back(path_node);
    visual_states[path_node.first][path_node.second] =
        SolverCellState::SOLUTION;
    if (path_node == endpoints.start) {
      break;
    }
    path_node = parents[path_node.first][path_node.second];
  }
  std::ranges::reverse(path);
  result.path_ = path;
  PushFrame(result, visual_states, path);
}

}  // namespace

auto GetGridSize(const MazeGrid& maze_grid) -> std::optional<GridSize> {
  const int kHeight = static_cast<int>(maze_grid.size());
  if (kHeight <= 0) {
    return std::nullopt;
  }
  const int kWidth = static_cast<int>(maze_grid[0].size());
  if (kWidth <= 0) {
    return std::nullopt;
  }
  return GridSize{.height = kHeight, .width = kWidth};
}

auto IsValidPosition(GridPosition pos, GridSize grid_size) -> bool {
  return pos.first >= 0 && pos.first < grid_size.height && pos.second >= 0 &&
         pos.second < grid_size.width;
}

auto CreateBoolGrid(GridSize grid_size, bool initial) -> BoolGrid {
  const auto kHeightSize = static_cast<size_t>(grid_size.height);
  const auto kWidthSize = static_cast<size_t>(grid_size.width);
  return {kHeightSize, std::vector<bool>(kWidthSize, initial)};
}

auto CreateIntGrid(GridSize grid_size, int initial) -> IntGrid {
  const auto kHeightSize = static_cast<size_t>(grid_size.height);
  const auto kWidthSize = static_cast<size_t>(grid_size.width);
  return {kHeightSize, std::vector<int>(kWidthSize, initial)};
}

auto CreateStateGrid(GridSize grid_size, SolverCellState initial) -> StateGrid {
  const auto kHeightSize = static_cast<size_t>(grid_size.height);
  const auto kWidthSize = static_cast<size_t>(grid_size.width);
  return {kHeightSize, std::vector<SolverCellState>(kWidthSize, initial)};
}

auto CreateParentGrid(GridSize grid_size, GridPosition initial) -> ParentGrid {
  const auto kHeightSize = static_cast<size_t>(grid_size.height);
  const auto kWidthSize = static_cast<size_t>(grid_size.width);
  return {kHeightSize, std::vector<GridPosition>(kWidthSize, initial)};
}

void PushFrame(SearchResult& result,
               const StateGrid& visual_states,
               const std::vector<GridPosition>& current_path) {
  SearchFrame frame;
  frame.visual_states_ = visual_states;
  frame.current_path_ = current_path;
  result.frames_.push_back(std::move(frame));
}

auto ShouldSaveFrameForCurrent(const ParentGrid& parents, GridPosition current,
                               const PathEndpoints& endpoints) -> bool {
  if (current == endpoints.end) {
    return true;
  }
  return !ShouldSkipStraightLineFrame(parents, current);
}

auto ShouldSaveBacktrackFrame(const ParentGrid& parents, GridPosition current,
                              const PathEndpoints& endpoints) -> bool {
  if (current == endpoints.start || current == endpoints.end) {
    return true;
  }
  return !ShouldSkipStraightLineFrame(parents, current);
}

auto CreateTrivialResult(GridSize grid_size, GridPosition node) -> SearchResult {
  SearchResult result;
  auto visual_states = CreateStateGrid(grid_size, SolverCellState::NONE);
  auto visited = CreateBoolGrid(grid_size, false);
  visited[node.first][node.second] = true;
  visual_states[node.first][node.second] = SolverCellState::SOLUTION;
  result.path_.push_back(node);
  PushFrame(result, visual_states, result.path_);
  result.explored_ = std::move(visited);
  result.found_ = true;
  return result;
}

void FinalizeSearchResult(bool found, const PathEndpoints& endpoints,
                          const ParentGrid& parents,
                          StateGrid& visual_states, BoolGrid&& visited,
                          SearchResult& result) {
  if (found) {
    AppendSolutionPath(endpoints, parents, visual_states, result);
  } else {
    PushFrame(result, visual_states, {});
  }

  result.found_ = found;
  result.explored_ = std::move(visited);
}

auto ManhattanDistance(PositionPair positions) -> int {
  return std::abs(positions.first.first - positions.second.first) +
         std::abs(positions.first.second - positions.second.second);
}

}  // namespace MazeSolverDomain::detail
