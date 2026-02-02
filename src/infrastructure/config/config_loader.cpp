#include "config_loader.h"

#include <exception>
#include <sstream>

#include "toml++/toml.hpp"

namespace ConfigLoader {

namespace {

constexpr int kDefaultMazeWidth = 10;
constexpr int kDefaultMazeHeight = 10;
constexpr int kDefaultUnitPixels = 15;
constexpr size_t kHexColorLength = 6;
constexpr int kHexBase = 16;

auto DefaultConfig() -> Config::AppConfig {
  Config::AppConfig config;
  config.maze.width = kDefaultMazeWidth;
  config.maze.height = kDefaultMazeHeight;
  config.maze.unit_pixels = kDefaultUnitPixels;
  config.maze.start_node = {0, 0};
  config.maze.end_node = {config.maze.height - 1, config.maze.width - 1};
  config.maze.generation_algorithms = {
      {MazeGeneration::MazeAlgorithmType::DFS,
       MazeGeneration::algorithm_name(MazeGeneration::MazeAlgorithmType::DFS)}};
  return config;
}

auto ParseHexColorString(std::string hex_string,
                         unsigned char* color_triplet,
                         std::vector<std::string>& warnings) -> bool {
  if (color_triplet == nullptr) {
    return false;
  }
  if (hex_string.empty()) {
    return false;
  }
  if (hex_string[0] == '#') {
    hex_string = hex_string.substr(1);
  }
  if (hex_string.length() != kHexColorLength) {
    warnings.push_back("Warning: Hex color string '#" + hex_string +
                       "' must be " + std::to_string(kHexColorLength) +
                       " characters long.");
    return false;
  }
  try {
    color_triplet[0] = static_cast<unsigned char>(
        std::stoul(hex_string.substr(0, 2), nullptr, kHexBase));
    color_triplet[1] = static_cast<unsigned char>(
        std::stoul(hex_string.substr(2, 2), nullptr, kHexBase));
    color_triplet[2] = static_cast<unsigned char>(
        std::stoul(hex_string.substr(4, 2), nullptr, kHexBase));
    return true;
  } catch (const std::exception&) {
    warnings.push_back("Warning: Invalid character in hex color string '#" +
                       hex_string + "'.");
    return false;
  }
}

}  // namespace

auto load_config(const std::string& filename) -> LoadResult {
  LoadResult result;
  result.config = DefaultConfig();

  toml::table config;
  try {
    config = toml::parse_file(filename);
  } catch (const toml::parse_error& err) {
    result.ok = false;
    std::ostringstream oss;
    oss << "Error parsing config file '" << filename << "':\n" << err;
    result.error = oss.str();
    return result;
  }

  result.config.maze.width =
      config["MazeConfig"]["MazeWidth"].value_or(result.config.maze.width);
  result.config.maze.height =
      config["MazeConfig"]["MazeHeight"].value_or(result.config.maze.height);
  result.config.maze.unit_pixels = config["MazeConfig"]["UnitPixels"].value_or(
      result.config.maze.unit_pixels);

  int start_y = config["MazeConfig"]["StartNodeY"].value_or(
      result.config.maze.start_node.first);
  int start_x = config["MazeConfig"]["StartNodeX"].value_or(
      result.config.maze.start_node.second);
  result.config.maze.start_node = {start_y, start_x};

  int end_y =
      config["MazeConfig"]["EndNodeY"].value_or(result.config.maze.height - 1);
  int end_x =
      config["MazeConfig"]["EndNodeX"].value_or(result.config.maze.width - 1);
  result.config.maze.end_node = {end_y, end_x};

  result.config.maze.generation_algorithms.clear();
  if (auto* algos = config["MazeConfig"]["GenerationAlgorithms"].as_array()) {
    for (const auto& elem : *algos) {
      if (auto algo_name = elem.value<std::string>()) {
        MazeGeneration::MazeAlgorithmType type;
        if (MazeGeneration::try_parse_algorithm(*algo_name, type)) {
          result.config.maze.generation_algorithms.push_back(
              {type, MazeGeneration::algorithm_name(type)});
        } else {
          result.warnings.push_back("Warning: Unknown generation algorithm '" +
                                    *algo_name + "' in config. Ignoring.");
        }
      }
    }
  }

  if (result.config.maze.generation_algorithms.empty()) {
    result.warnings.emplace_back(
        "Info: No valid generation algorithms specified. Defaulting to DFS.");
    result.config.maze.generation_algorithms.push_back(
        {MazeGeneration::MazeAlgorithmType::DFS,
         MazeGeneration::algorithm_name(
             MazeGeneration::MazeAlgorithmType::DFS)});
  }

  result.config.maze.search_algorithms.clear();
  if (auto* search_algos =
          config["MazeConfig"]["SearchAlgorithms"].as_array()) {
    for (const auto& elem : *search_algos) {
      if (auto algo_name = elem.value<std::string>()) {
        MazeSolverDomain::SolverAlgorithmType type;
        if (MazeSolverDomain::TryParseAlgorithm(*algo_name, type)) {
          result.config.maze.search_algorithms.push_back(
              {type, MazeSolverDomain::AlgorithmName(type)});
        } else {
          result.warnings.push_back("Warning: Unknown search algorithm '" +
                                    *algo_name + "' in config. Ignoring.");
        }
      }
    }
  }

  if (result.config.maze.search_algorithms.empty()) {
    result.ok = false;
    result.error = "SearchAlgorithms 不能为空 (cannot be empty).";
  }

  auto load_color = [&](const char* key, unsigned char* color_arr) -> void {
    if (auto color_str = config["ColorConfig"][key].value<std::string>()) {
      ParseHexColorString(*color_str, color_arr, result.warnings);
    }
  };

  load_color("BackgroundColor", result.config.colors.background);
  load_color("OuterWallColor", result.config.colors.outer_wall);
  load_color("InnerWallColor", result.config.colors.inner_wall);
  load_color("StartNodeColor", result.config.colors.start);
  load_color("EndNodeColor", result.config.colors.end);
  load_color("FrontierColor", result.config.colors.frontier);
  load_color("VisitedColor", result.config.colors.visited);
  load_color("CurrentProcessingColor", result.config.colors.current);
  load_color("SolutionPathColor", result.config.colors.solution_path);

  return result;
}

}  // namespace ConfigLoader
