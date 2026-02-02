#include "cli/commands/search_algorithms_command.h"

#include <cctype>
#include <sstream>

#include "application/services/maze_solver.h"

namespace Cli {

namespace {

auto Trim(std::string_view value) -> std::string {
  size_t start = 0;
  while (start < value.size() &&
         (std::isspace(static_cast<unsigned char>(value[start])) != 0)) {
    ++start;
  }
  size_t end = value.size();
  while (end > start &&
         (std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)) {
    --end;
  }
  return std::string(value.substr(start, end - start));
}

auto SplitAlgorithms(const std::vector<std::string>& args)
    -> std::vector<std::string> {
  std::vector<std::string> result;
  for (const auto& arg : args) {
    std::stringstream stream(arg);
    std::string token;
    while (std::getline(stream, token, ',')) {
      std::string trimmed = Trim(token);
      if (!trimmed.empty()) {
        result.push_back(trimmed);
      }
    }
  }
  return result;
}

}  // namespace

void PrintSupportedSearchAlgorithms(std::ostream& out) {
  const auto kSupported = MazeSolver::SupportedAlgorithms();
  out << "Supported SearchAlgorithms: ";
  for (size_t i = 0; i < kSupported.size(); ++i) {
    out << kSupported[i];
    if (i < kSupported.size() - 1) {
      out << ", ";
    }
  }
  out << "\n";
}

auto ApplySearchAlgorithms(const std::vector<std::string>& args,
                           CommandContext& ctx) -> int {
  if (args.empty()) {
    return 1;
  }

  std::vector<std::string> tokens = SplitAlgorithms(args);
  if (tokens.empty()) {
    ctx.err << "No algorithms provided.\n";
    return 1;
  }

  std::vector<Config::SearchAlgorithmInfo> selected;
  for (const auto& token : tokens) {
    MazeSolver::SolverAlgorithmType type;
    if (!MazeSolver::TryParseAlgorithm(token, type)) {
      ctx.err << "Unknown algorithm: " << token << "\n";
      return 1;
    }
    selected.push_back({type, MazeSolver::AlgorithmName(type)});
  }

  ctx.config.maze.search_algorithms = std::move(selected);
  ctx.out << "SearchAlgorithms set to: ";
  for (size_t i = 0; i < ctx.config.maze.search_algorithms.size(); ++i) {
    ctx.out << ctx.config.maze.search_algorithms[i].name;
    if (i < ctx.config.maze.search_algorithms.size() - 1) {
      ctx.out << ", ";
    }
  }
  ctx.out << "\n";
  return 0;
}

namespace {

auto HandleSearchAlgorithms(const std::vector<std::string>& args,
                            CommandContext& ctx) -> int {
  if (args.empty()) {
    ctx.out << "SearchAlgorithms: ";
    for (size_t i = 0; i < ctx.config.maze.search_algorithms.size(); ++i) {
      ctx.out << ctx.config.maze.search_algorithms[i].name;
      if (i < ctx.config.maze.search_algorithms.size() - 1) {
        ctx.out << ", ";
      }
    }
    ctx.out << "\n";
    PrintSupportedSearchAlgorithms(ctx.out);
    return 0;
  }

  return ApplySearchAlgorithms(args, ctx);
}

}  // namespace

void RegisterSearchAlgorithmsCommand(CliApp& app) {
  Command command;
  command.name = "search-algorithms";
  command.description = "Override SearchAlgorithms (comma-separated)";
  command.handler = HandleSearchAlgorithms;
  command.exit_after = true;
  app.register_command(command);

  Command alias = command;
  alias.name = "search-algos";
  app.register_command(std::move(alias));
}

}  // namespace Cli
