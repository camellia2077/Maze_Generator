#include "domain/maze_solver.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <limits>
#include <optional>
#include <queue>
#include <stack>

namespace {

using MazeGrid = MazeDomain::MazeGrid;
using GridPosition = MazeSolverDomain::GridPosition;
using SearchFrame = MazeSolverDomain::SearchFrame;
using SearchResult = MazeSolverDomain::SearchResult;
using SolverCellState = MazeSolverDomain::SolverCellState;
using SolverAlgorithmType = MazeSolverDomain::SolverAlgorithmType;
using BoolGrid = std::vector<std::vector<bool>>;
using IntGrid = std::vector<std::vector<int>>;
using StateGrid = std::vector<std::vector<SolverCellState>>;
using ParentGrid = std::vector<std::vector<GridPosition>>;

struct GridSize {
  int height;
  int width;
};

constexpr int kWallCount = MazeDomain::kWallCount;
constexpr int kInvalidCoord = -1;
constexpr GridPosition kInvalidCell = {kInvalidCoord, kInvalidCoord};
constexpr int kWallTop = 0;
constexpr int kWallRight = 1;
constexpr int kWallBottom = 2;
constexpr int kWallLeft = 3;
constexpr int kMaxCostDivisor = 4;

struct PathEndpoints {
  GridPosition start;
  GridPosition end;
};

struct PositionPair {
  GridPosition first;
  GridPosition second;
};

struct StraightLineTriplet {
  GridPosition first;
  GridPosition second;
  GridPosition third;
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

auto ShouldSkipStraightLineFrame(
    const ParentGrid& parents, GridPosition current) -> bool {
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

auto ShouldSaveFrameForCurrent(const ParentGrid& parents, GridPosition current,
                               const PathEndpoints& endpoints) -> bool {
  if (current == endpoints.end) {
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

auto TryPushDfsNeighbor(const MazeGrid& maze_grid, GridSize grid_size,
                        GridPosition current,
                        const DirectionDeltas& deltas,
                        const BoolGrid& visited, ParentGrid& parents,
                        std::stack<GridPosition>& frontier,
                        StateGrid& visual_states) -> bool {
  for (int dir_index = 0; dir_index < kWallCount; ++dir_index) {
    const int kNextR = current.first + deltas.row_delta[dir_index];
    const int kNextC = current.second + deltas.col_delta[dir_index];

    const bool kWallExists = maze_grid[current.first][current.second]
                                 .walls[deltas.wall_check_index[dir_index]];

    if (kNextR >= 0 && kNextR < grid_size.height && kNextC >= 0 &&
        kNextC < grid_size.width && !visited[kNextR][kNextC] &&
        !kWallExists) {
      parents[kNextR][kNextC] = current;
      frontier.emplace(kNextR, kNextC);
      visual_states[kNextR][kNextC] = SolverCellState::FRONTIER;
      return true;
    }
  }
  return false;
}

auto ShouldSaveBacktrackFrame(const ParentGrid& parents, GridPosition current,
                              const PathEndpoints& endpoints) -> bool {
  if (current == endpoints.start || current == endpoints.end) {
    return true;
  }
  return !ShouldSkipStraightLineFrame(parents, current);
}

auto ManhattanDistance(PositionPair positions) -> int {
  return std::abs(positions.first.first - positions.second.first) +
         std::abs(positions.first.second - positions.second.second);
}

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

void EnqueueAStarNeighbors(
    const MazeGrid& maze_grid, GridSize grid_size,
    const SearchTargets& targets, const DirectionDeltas& deltas,
    const BoolGrid& visited, IntGrid& g_scores, ParentGrid& parents,
    std::priority_queue<AStarNode, std::vector<AStarNode>, AStarNodeCompare>&
        frontier,
    StateGrid& visual_states) {
  for (int dir_index = 0; dir_index < kWallCount; ++dir_index) {
    const int kNextR = targets.current.first + deltas.row_delta[dir_index];
    const int kNextC = targets.current.second + deltas.col_delta[dir_index];

    const bool kWallExists = maze_grid[targets.current.first]
                                 [targets.current.second]
                                 .walls[deltas.wall_check_index[dir_index]];

    if (kNextR >= 0 && kNextR < grid_size.height && kNextC >= 0 &&
        kNextC < grid_size.width && !visited[kNextR][kNextC] && !kWallExists) {
      const int kTentativeG =
          g_scores[targets.current.first][targets.current.second] + 1;
      if (kTentativeG < g_scores[kNextR][kNextC]) {
        g_scores[kNextR][kNextC] = kTentativeG;
        parents[kNextR][kNextC] = targets.current;
        const GridPosition kNextPos = {kNextR, kNextC};
        const PositionPair kNextToEnd{
            .first = kNextPos, .second = targets.end};
        const int kFScore = kTentativeG + ManhattanDistance(kNextToEnd);
        frontier.push({kFScore, kTentativeG, {kNextR, kNextC}});
        visual_states[kNextR][kNextC] = SolverCellState::FRONTIER;
      }
    }
  }
}

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

auto SolveDfs(const MazeGrid& maze_grid, GridPosition start_node,
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

  std::stack<GridPosition> frontier;
  frontier.push(start_node);
  visual_states[start_node.first][start_node.second] =
      SolverCellState::FRONTIER;
  PushFrame(result, visual_states, {});

  constexpr std::array<int, kWallCount> kRowDelta = {-1, 0, 1, 0};
  constexpr std::array<int, kWallCount> kColDelta = {0, 1, 0, -1};
  constexpr std::array<int, kWallCount> kWallCheckIndex = {
      kWallTop, kWallRight, kWallBottom, kWallLeft};
  const DirectionDeltas kDeltas{.row_delta = kRowDelta,
                                .col_delta = kColDelta,
                                .wall_check_index = kWallCheckIndex};

  bool found = false;
  while (!frontier.empty() && !found) {
    const GridPosition kCurrent = frontier.top();

    if (visual_states[kCurrent.first][kCurrent.second] ==
        SolverCellState::VISITED_PROC) {
      frontier.pop();
      continue;
    }

    bool should_save_frame = true;
    if (!visited[kCurrent.first][kCurrent.second]) {
      visited[kCurrent.first][kCurrent.second] = true;
      visual_states[kCurrent.first][kCurrent.second] =
          SolverCellState::CURRENT_PROC;

      should_save_frame =
          ShouldSaveFrameForCurrent(parents, kCurrent, kEndpoints);

      if (should_save_frame) {
        PushFrame(result, visual_states, {});
      }
    }

    if (kCurrent == end_node) {
      found = true;
    }

    if (found) {
      break;
    }

    const bool kPushedNeighbor =
        TryPushDfsNeighbor(maze_grid, *kGridSize, kCurrent, kDeltas, visited,
                           parents, frontier, visual_states);

    if (!kPushedNeighbor) {
      frontier.pop();
      visual_states[kCurrent.first][kCurrent.second] =
          SolverCellState::VISITED_PROC;

      if (ShouldSaveBacktrackFrame(parents, kCurrent, kEndpoints)) {
        PushFrame(result, visual_states, {});
      }
    }
  }

  FinalizeSearchResult(found, kEndpoints, parents, visual_states,
                       std::move(visited), result);
  return result;
}

auto SolveAStar(const MazeGrid& maze_grid, GridPosition start_node,
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
  const PositionPair kStartEnd{.first = start_node, .second = end_node};
  frontier.push({ManhattanDistance(kStartEnd), 0, start_node});
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

    const SearchTargets kTargets{.current = kCurrent, .end = end_node};
    EnqueueAStarNeighbors(maze_grid, *kGridSize, kTargets, kDeltas, visited,
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

}  // namespace

namespace MazeSolverDomain {

MazeSolverFactory::MazeSolverFactory() {
  RegisterSolver(SolverAlgorithmType::BFS, "BFS", SolveBfs);
  RegisterSolver(SolverAlgorithmType::DFS, "DFS", SolveDfs);
  RegisterSolver(SolverAlgorithmType::ASTAR, "AStar", SolveAStar);
  name_to_type_["A*"] = SolverAlgorithmType::ASTAR;
}

auto MazeSolverFactory::Instance() -> MazeSolverFactory& {
  static MazeSolverFactory instance;
  return instance;
}

void MazeSolverFactory::RegisterSolver(SolverAlgorithmType type,
                                       std::string name, Solver solver) {
  registry_[type] = Entry{.name = std::move(name), .solver = std::move(solver)};

  std::string key;
  key.reserve(registry_[type].name.size());
  for (unsigned char character : registry_[type].name) {
    key.push_back(static_cast<char>(std::toupper(character)));
  }
  name_to_type_[key] = type;
}

auto MazeSolverFactory::HasSolver(SolverAlgorithmType type) const -> bool {
  return registry_.contains(type);
}

auto MazeSolverFactory::GetSolver(SolverAlgorithmType type) const
    -> MazeSolverFactory::Solver {
  auto iterator = registry_.find(type);
  if (iterator != registry_.end()) {
    return iterator->second.solver;
  }
  return {};
}

auto MazeSolverFactory::NameFor(SolverAlgorithmType type) const -> std::string {
  auto iterator = registry_.find(type);
  if (iterator != registry_.end()) {
    return iterator->second.name;
  }
  return {};
}

auto MazeSolverFactory::TryParse(std::string_view name,
                                 SolverAlgorithmType& out_type) const -> bool {
  std::string key;
  key.reserve(name.size());
  for (unsigned char character : name) {
    key.push_back(static_cast<char>(std::toupper(character)));
  }
  auto iterator = name_to_type_.find(key);
  if (iterator == name_to_type_.end()) {
    return false;
  }
  out_type = iterator->second;
  return true;
}

auto Solve(const MazeGrid& maze_grid, GridPosition start_node,
           GridPosition end_node, SolverAlgorithmType algorithm_type)
    -> SearchResult {
  auto solver = MazeSolverFactory::Instance().GetSolver(algorithm_type);
  if (!solver) {
    solver = MazeSolverFactory::Instance().GetSolver(SolverAlgorithmType::BFS);
  }
  if (!solver) {
    return {};
  }
  return solver(maze_grid, start_node, end_node);
}

auto AlgorithmName(SolverAlgorithmType algorithm_type) -> std::string {
  return MazeSolverFactory::Instance().NameFor(algorithm_type);
}

auto TryParseAlgorithm(std::string_view name, SolverAlgorithmType& out_type)
    -> bool {
  return MazeSolverFactory::Instance().TryParse(name, out_type);
}

}  // namespace MazeSolverDomain
