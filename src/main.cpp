#include <chrono>  // Required for high-precision timing
#include <filesystem>
#include <iomanip>  // Required for std::fixed and std::setprecision
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "application/services/maze_generation.h"
#include "application/services/maze_solver.h"
#include "cli/commands/generation_algorithms_command.h"
#include "cli/commands/search_algorithms_command.h"
#include "cli/commands/version_command.h"
#include "cli/framework/cli_app.h"
#include "infrastructure/config/config_loader.h"
#include "infrastructure/graphics/maze_renderer.h"

// ANSI escape codes for colors
const std::string kResetColor = "\033[0m";
const std::string kGreenColor = "\033[32m";
const std::string kConfigFilename = "config.toml";
const std::string kConfigDirname = "config";

namespace {

using Clock = std::chrono::high_resolution_clock;

auto BuildConfigPath(const char* argv0) -> std::filesystem::path {
  const std::filesystem::path kExePath = std::filesystem::absolute(argv0);
  return kExePath.parent_path() / kConfigDirname / kConfigFilename;
}

void PrintAlgorithmList(
    const std::vector<Config::AlgorithmInfo>& algorithms) {
  for (size_t index = 0; index < algorithms.size(); ++index) {
    std::cout << algorithms[index].name;
    if (index + 1 < algorithms.size()) {
      std::cout << ", ";
    }
  }
  std::cout << std::endl;
}

void PrintSearchAlgorithmList(
    const std::vector<Config::SearchAlgorithmInfo>& algorithms) {
  for (size_t index = 0; index < algorithms.size(); ++index) {
    std::cout << algorithms[index].name;
    if (index + 1 < algorithms.size()) {
      std::cout << ", ";
    }
  }
  std::cout << std::endl;
}

void PrintConfigSummary(const Config::AppConfig& config,
                        const std::filesystem::path& config_path) {
  std::cout << "Configuration successfully loaded from "
            << config_path.string() << std::endl;
  std::cout << "Maze Dimensions: " << config.maze.width << "x"
            << config.maze.height
            << ", Unit Pixels: " << config.maze.unit_pixels << std::endl;
  std::cout << "Start Node: (" << config.maze.start_node.first << ","
            << config.maze.start_node.second << "), End Node: ("
            << config.maze.end_node.first << ","
            << config.maze.end_node.second << ")" << std::endl;
  std::cout << "Selected Generation Algorithms: ";
  PrintAlgorithmList(config.maze.generation_algorithms);
  std::cout << "Selected Search Algorithms: ";
  PrintSearchAlgorithmList(config.maze.search_algorithms);
}

void PrintLoadWarnings(const std::vector<std::string>& warnings) {
  for (const auto& warning : warnings) {
    std::cerr << warning << "\n";
  }
}

auto IsStartNodeValid(const Config::MazeConfig& maze) -> bool {
  return maze.start_node.first >= 0 &&
         maze.start_node.first < maze.height &&
         maze.start_node.second >= 0 &&
         maze.start_node.second < maze.width;
}

auto ResolveStartNode(const Config::MazeConfig& maze) -> std::pair<int, int> {
  if (IsStartNodeValid(maze)) {
    return maze.start_node;
  }
  return {0, 0};
}

void MaybeLogAdjustedStart(const Config::MazeConfig& maze,
                           const Config::AlgorithmInfo& algo_info,
                           int start_row,
                           int start_col) {
  if (IsStartNodeValid(maze)) {
    return;
  }

  if (algo_info.type == MazeGeneration::MazeAlgorithmType::DFS ||
      algo_info.type == MazeGeneration::MazeAlgorithmType::PRIMS ||
      algo_info.type == MazeGeneration::MazeAlgorithmType::GROWING_TREE) {
    std::cout << "Adjusted maze generation start point to (" << start_row
              << "," << start_col
              << ") due to out-of-bounds config START_NODE for DFS/Prims/Growing Tree."
              << std::endl;
  }
}

auto PrepareMazeGrid(const Config::MazeConfig& maze)
    -> MazeGeneration::MazeGrid {
  const auto kGridHeight = static_cast<size_t>(maze.height);
  const auto kGridWidth = static_cast<size_t>(maze.width);
  return {kGridHeight, MazeGeneration::MazeGrid::value_type(kGridWidth)};
}

void RunSolverAndRender(const MazeGeneration::MazeGrid& maze_grid,
                        const Config::AlgorithmInfo& algo_info,
                        const Config::AppConfig& config,
                        MazeSolver::SolverAlgorithmType solver_type,
                        const char* solver_label) {
  std::cout << "--- " << solver_label << " Solving & Image Generation ("
            << algo_info.name << ") ---" << std::endl;
  const auto kStartTime = Clock::now();
  const auto kResult = MazeSolver::Solve(maze_grid, solver_type, config);
  const auto kRenderResult =
      MazeSolver::RenderSearchResult(kResult, maze_grid, solver_type,
                                     algo_info.name, config);
  const std::string kSolverName = MazeSolver::AlgorithmName(solver_type);
  const std::string kDisplayName =
      kSolverName.empty() ? "Solver" : kSolverName;
  if (!kRenderResult.ok) {
    std::cerr << kDisplayName << ": " << kRenderResult.error << std::endl;
  } else {
    std::cout << "Rendered " << kRenderResult.frames_written << " frames"
              << " for " << kDisplayName << " (maze generated by "
              << algo_info.name << ") in " << kRenderResult.output_folder
              << std::endl;
  }
  const auto kEndTime = Clock::now();
  const auto kTimeTaken =
      std::chrono::duration<double>(kEndTime - kStartTime);
  std::cout << kGreenColor << std::fixed << std::setprecision(3)
            << "Time for " << solver_label
            << " solving & image generation: " << kTimeTaken.count() << " s"
            << kResetColor << std::endl;
}

void RunGenerationForAlgorithm(const Config::AppConfig& config,
                               const Config::AlgorithmInfo& algo_info) {
  std::cout << "\n--- Processing for Maze Generation Algorithm: "
            << algo_info.name << " ---" << std::endl;

  auto maze_grid = PrepareMazeGrid(config.maze);

  std::cout << "--- Maze Generation (" << algo_info.name << ") ---"
            << std::endl;
  const auto kStartNode = ResolveStartNode(config.maze);
  const int kGenStartRow = kStartNode.first;
  const int kGenStartCol = kStartNode.second;
  MaybeLogAdjustedStart(config.maze, algo_info, kGenStartRow, kGenStartCol);

  const auto kStartTime = Clock::now();
  MazeGeneration::generate_maze_structure(maze_grid, kGenStartRow,
                                          kGenStartCol, config.maze.width,
                                          config.maze.height, algo_info.type);
  const auto kEndTime = Clock::now();
  const auto kTimeTaken =
      std::chrono::duration<double>(kEndTime - kStartTime);
  std::cout << kGreenColor << std::fixed << std::setprecision(3)
            << "Time for maze generation: " << kTimeTaken.count() << " s"
            << kResetColor << std::endl;

  std::cout << "Maze generated." << std::endl;

  for (const auto& solver_info : config.maze.search_algorithms) {
    RunSolverAndRender(maze_grid, algo_info, config, solver_info.type,
                       solver_info.name.c_str());
  }
}

void RunGenerationPipeline(const Config::AppConfig& config) {
  for (const auto& algo_info : config.maze.generation_algorithms) {
    RunGenerationForAlgorithm(config, algo_info);
  }
}

void RegisterBuiltInCommands(Cli::CliApp& cli) {
  cli.register_command({.name = "run",
                        .description = "Run maze generation + solving pipeline",
                        .handler = [](const std::vector<std::string>& args,
                                      Cli::CommandContext& ctx) -> int {
                          if (!args.empty()) {
                            ctx.err << "Unknown option: " << args.front()
                                    << "\n";
                            return 1;
                          }
                          return 0;
                        },
                        .exit_after = false});
  cli.register_command({.name = "help",
                        .description = "Show available commands",
                        .handler = [&cli](const std::vector<std::string>&,
                                          Cli::CommandContext& ctx) -> int {
                          cli.print_help(ctx.out);
                          return 0;
                        }});
}

auto RunCli(Cli::CliApp& cli,
            int argc,
            char** argv,
            Config::AppConfig& config,
            int& exit_code) -> bool {
  Cli::CommandContext cli_ctx{
      .config = config, .out = std::cout, .err = std::cerr};
  bool handled = false;
  exit_code = cli.run(argc, argv, cli_ctx, handled);
  return handled;
}

}  // namespace

auto main(int argc, char** argv) -> int {
  std::cout << "--- Parameter Loading ---" << std::endl;
  const auto kStartTimeConfig = Clock::now();

  const auto kConfigPath = BuildConfigPath(argv[0]);
  ConfigLoader::LoadResult load_result =
      ConfigLoader::load_config(kConfigPath.string());
  Config::AppConfig config = std::move(load_result.config);
  if (!load_result.ok) {
    std::cerr << load_result.error << "\n";
  } else {
    PrintConfigSummary(config, kConfigPath);
  }
  PrintLoadWarnings(load_result.warnings);

  if (config.maze.search_algorithms.empty()) {
    return 1;
  }

  if (!load_result.ok) {
    std::cerr << "Using default values.\n";
  }

  Cli::CliApp cli;
  Cli::RegisterVersionCommand(cli);
  Cli::RegisterGenerationAlgorithmsCommand(cli);
  Cli::RegisterSearchAlgorithmsCommand(cli);
  RegisterBuiltInCommands(cli);

  int cli_code = 0;
  if (RunCli(cli, argc, argv, config, cli_code)) {
    return cli_code;
  }

  const auto kEndTimeConfig = Clock::now();
  const auto kTimeTakenConfig =
      std::chrono::duration<double>(kEndTimeConfig - kStartTimeConfig);
  std::cout << kGreenColor << std::fixed << std::setprecision(3)
            << "Time to load config: " << kTimeTakenConfig.count() << " s"
            << kResetColor << std::endl;

  RunGenerationPipeline(config);

  std::cout << "\n--- Processing Complete ---" << std::endl;
  std::cout << "All selected algorithms processed. Images saved in respective "
               "folders."
            << std::endl;

  return 0;
}
