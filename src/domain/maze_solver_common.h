#ifndef MAZE_DOMAIN_MAZE_SOLVER_COMMON_H
#define MAZE_DOMAIN_MAZE_SOLVER_COMMON_H

#include <array>
#include <optional>
#include <utility>
#include <vector>

#include "domain/maze_solver.h"

namespace MazeSolverDomain::detail {

using MazeGrid = MazeDomain::MazeGrid;
using GridPosition = MazeSolverDomain::GridPosition;
using SearchFrame = MazeSolverDomain::SearchFrame;
using SearchResult = MazeSolverDomain::SearchResult;
using SolverCellState = MazeSolverDomain::SolverCellState;
using BoolGrid = std::vector<std::vector<bool>>;
using IntGrid = std::vector<std::vector<int>>;
using StateGrid = std::vector<std::vector<SolverCellState>>;
using ParentGrid = std::vector<std::vector<GridPosition>>;

struct GridSize {
  int height;
  int width;
};

inline constexpr int kWallCount = MazeDomain::kWallCount;
inline constexpr int kInvalidCoord = -1;
inline constexpr GridPosition kInvalidCell = {kInvalidCoord, kInvalidCoord};
inline constexpr int kWallTop = 0;
inline constexpr int kWallRight = 1;
inline constexpr int kWallBottom = 2;
inline constexpr int kWallLeft = 3;
inline constexpr int kMaxCostDivisor = 4;

struct PathEndpoints {
  GridPosition start;
  GridPosition end;
};

struct PositionPair {
  GridPosition first;
  GridPosition second;
};

struct DirectionDeltas {
  std::array<int, kWallCount> row_delta;
  std::array<int, kWallCount> col_delta;
  std::array<int, kWallCount> wall_check_index;
};

struct SearchTargets {
  GridPosition current;
  GridPosition end;
};

struct AStarNode {
  int f_score;
  int g_score;
  GridPosition pos;
};

struct AStarNodeCompare {
  auto operator()(const AStarNode& left, const AStarNode& right) const -> bool {
    if (left.f_score != right.f_score) {
      return left.f_score > right.f_score;
    }
    return left.g_score > right.g_score;
  }
};

auto GetGridSize(const MazeGrid& maze_grid) -> std::optional<GridSize>;
auto IsValidPosition(GridPosition pos, GridSize grid_size) -> bool;
auto CreateBoolGrid(GridSize grid_size, bool initial) -> BoolGrid;
auto CreateIntGrid(GridSize grid_size, int initial) -> IntGrid;
auto CreateStateGrid(GridSize grid_size, SolverCellState initial) -> StateGrid;
auto CreateParentGrid(GridSize grid_size, GridPosition initial) -> ParentGrid;
void PushFrame(SearchResult& result, const StateGrid& visual_states,
               const std::vector<GridPosition>& current_path);
auto ShouldSaveFrameForCurrent(const ParentGrid& parents, GridPosition current,
                               const PathEndpoints& endpoints) -> bool;
auto ShouldSaveBacktrackFrame(const ParentGrid& parents, GridPosition current,
                              const PathEndpoints& endpoints) -> bool;
auto CreateTrivialResult(GridSize grid_size, GridPosition node) -> SearchResult;
void FinalizeSearchResult(bool found, const PathEndpoints& endpoints,
                          const ParentGrid& parents, StateGrid& visual_states,
                          BoolGrid&& visited, SearchResult& result);
auto ManhattanDistance(PositionPair positions) -> int;

}  // namespace MazeSolverDomain::detail

#endif  // MAZE_DOMAIN_MAZE_SOLVER_COMMON_H
