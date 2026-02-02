#ifndef MAZE_DOMAIN_MAZE_SOLVER_H
#define MAZE_DOMAIN_MAZE_SOLVER_H

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "domain/maze_grid.h"

namespace MazeSolverDomain {

using GridPosition = std::pair<int, int>;

enum class SolverAlgorithmType { BFS, DFS, ASTAR, DIJKSTRA, GREEDY_BEST_FIRST };

enum class SolverCellState {
  NONE,
  START,
  END,
  FRONTIER,
  CURRENT_PROC,
  VISITED_PROC,
  SOLUTION
};

struct SearchFrame {
  std::vector<std::vector<SolverCellState>> visual_states_;
  std::vector<GridPosition> current_path_;
};

struct SearchResult {
  bool found_ = false;
  std::vector<GridPosition> path_;
  std::vector<std::vector<bool>> explored_;
  std::vector<SearchFrame> frames_;
};

class MazeSolverFactory {
 public:
  using Solver = std::function<SearchResult(const MazeDomain::MazeGrid&,
                                            GridPosition start_node,
                                            GridPosition end_node)>;

  static MazeSolverFactory& Instance();

  void RegisterSolver(SolverAlgorithmType type, std::string name,
                      Solver solver);
  bool HasSolver(SolverAlgorithmType type) const;
  Solver GetSolver(SolverAlgorithmType type) const;
  std::string NameFor(SolverAlgorithmType type) const;
  bool TryParse(std::string_view name, SolverAlgorithmType& out_type) const;
  std::vector<std::string> Names() const;

 private:
  MazeSolverFactory();

  struct Entry {
    std::string name;
    Solver solver;
  };

  std::map<SolverAlgorithmType, Entry> registry_;
  std::map<std::string, SolverAlgorithmType> name_to_type_;
};

SearchResult Solve(const MazeDomain::MazeGrid& maze_grid,
                   GridPosition start_node, GridPosition end_node,
                   SolverAlgorithmType algorithm_type);
std::string AlgorithmName(SolverAlgorithmType algorithm_type);
bool TryParseAlgorithm(std::string_view name, SolverAlgorithmType& out_type);
std::vector<std::string> supported_algorithms();

}  // namespace MazeSolverDomain

#endif  // MAZE_DOMAIN_MAZE_SOLVER_H
