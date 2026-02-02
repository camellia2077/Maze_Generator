#include "cli/commands/generation_algorithms_command.h"

#include <cctype>
#include <sstream>

#include "application/services/maze_generation.h"

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

void PrintSupportedGenerationAlgorithms(std::ostream& out) {
  const auto kSupported = MazeGeneration::supported_algorithms();
  out << "Supported GenerationAlgorithms: ";
  for (size_t i = 0; i < kSupported.size(); ++i) {
    out << kSupported[i];
    if (i < kSupported.size() - 1) {
      out << ", ";
    }
  }
  out << "\n";
}

auto ApplyGenerationAlgorithms(const std::vector<std::string>& args,
                               CommandContext& ctx) -> int {
  if (args.empty()) {
    return 1;
  }

  std::vector<std::string> tokens = SplitAlgorithms(args);
  if (tokens.empty()) {
    ctx.err << "No algorithms provided.\n";
    return 1;
  }

  std::vector<Config::AlgorithmInfo> selected;
  for (const auto& token : tokens) {
    MazeGeneration::MazeAlgorithmType type;
    if (!MazeGeneration::try_parse_algorithm(token, type)) {
      ctx.err << "Unknown algorithm: " << token << "\n";
      return 1;
    }
    selected.push_back({type, MazeGeneration::algorithm_name(type)});
  }

  ctx.config.maze.generation_algorithms = std::move(selected);
  ctx.out << "GenerationAlgorithms set to: ";
  for (size_t i = 0; i < ctx.config.maze.generation_algorithms.size(); ++i) {
    ctx.out << ctx.config.maze.generation_algorithms[i].name;
    if (i < ctx.config.maze.generation_algorithms.size() - 1) {
      ctx.out << ", ";
    }
  }
  ctx.out << "\n";
  return 0;
}

namespace {

auto HandleGenerationAlgorithms(const std::vector<std::string>& args,
                                CommandContext& ctx) -> int {
  if (args.empty()) {
    ctx.out << "GenerationAlgorithms: ";
    for (size_t i = 0; i < ctx.config.maze.generation_algorithms.size(); ++i) {
      ctx.out << ctx.config.maze.generation_algorithms[i].name;
      if (i < ctx.config.maze.generation_algorithms.size() - 1) {
        ctx.out << ", ";
      }
    }
    ctx.out << "\n";
    PrintSupportedGenerationAlgorithms(ctx.out);
    return 0;
  }

  return ApplyGenerationAlgorithms(args, ctx);
}

}  // namespace

void RegisterGenerationAlgorithmsCommand(CliApp& app) {
  Command command;
  command.name = "generation-algorithms";
  command.description = "Override GenerationAlgorithms (comma-separated)";
  command.handler = HandleGenerationAlgorithms;
  command.exit_after = true;
  app.register_command(command);

  Command alias = command;
  alias.name = "gen-algorithms";
  app.register_command(std::move(alias));

  Command show_alias = command;
  show_alias.name = "show";
  app.register_command(std::move(show_alias));
}

}  // namespace Cli
