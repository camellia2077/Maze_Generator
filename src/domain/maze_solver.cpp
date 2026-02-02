#include "domain/maze_solver.h"

#include <cctype>

#include "domain/maze_solver_algorithms.h"

namespace MazeSolverDomain {

using MazeGrid = MazeDomain::MazeGrid;

MazeSolverFactory::MazeSolverFactory() {
  RegisterSolver(SolverAlgorithmType::BFS, "BFS", detail::SolveBfs);
  RegisterSolver(SolverAlgorithmType::DFS, "DFS", detail::SolveDfs);
  RegisterSolver(SolverAlgorithmType::ASTAR, "AStar", detail::SolveAStar);
  RegisterSolver(SolverAlgorithmType::DIJKSTRA, "Dijkstra",
                 detail::SolveDijkstra);
  RegisterSolver(SolverAlgorithmType::GREEDY_BEST_FIRST, "Greedy Best-First",
                 detail::SolveGreedyBestFirst);
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

auto MazeSolverFactory::Names() const -> std::vector<std::string> {
  std::vector<std::string> result;
  result.reserve(registry_.size());
  for (const auto& entry : registry_) {
    result.push_back(entry.second.name);
  }
  return result;
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

auto supported_algorithms() -> std::vector<std::string> {
  return MazeSolverFactory::Instance().Names();
}

}  // namespace MazeSolverDomain
