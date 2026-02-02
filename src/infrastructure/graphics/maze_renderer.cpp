#include "infrastructure/graphics/maze_renderer.h"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace fs = std::filesystem;

namespace {

using GridPosition = MazeSolverDomain::GridPosition;
using SearchFrame = MazeSolverDomain::SearchFrame;
using SearchResult = MazeSolverDomain::SearchResult;
using SolverAlgorithmType = MazeSolverDomain::SolverAlgorithmType;
using SolverCellState = MazeSolverDomain::SolverCellState;

constexpr int kRgbChannels = 3;
constexpr int kGridSpacing = 2;
constexpr int kFrameIndexWidth = 4;
constexpr int kWallTop = 0;
constexpr int kWallRight = 1;
constexpr int kWallBottom = 2;

auto SolverFolderPrefix(SolverAlgorithmType algorithm_type) -> std::string {
  switch (algorithm_type) {
    case SolverAlgorithmType::BFS:
      return "bfs_frames_generated_by_";
    case SolverAlgorithmType::DFS:
      return "dfs_frames_generated_by_";
    case SolverAlgorithmType::ASTAR:
      return "astar_frames_generated_by_";
  }
  return "solver_frames_generated_by_";
}

struct ImageSize {
  int img_width_units;
  int img_height_units;
  int final_img_width;
  int final_img_height;
  size_t pixel_count;
};

struct ImageContext {
  int final_img_width;
  int final_img_height;
  int unit_pixels;
};

struct UnitCoord {
  int row;
  int col;
};

struct MazeCoord {
  int row;
  int col;
};

auto ComputeImageSize(const Config::MazeConfig& maze) -> ImageSize {
  const int kImgWidthUnits = (kGridSpacing * maze.width) + 1;
  const int kImgHeightUnits = (kGridSpacing * maze.height) + 1;
  const int kFinalImgWidth = kImgWidthUnits * maze.unit_pixels;
  const int kFinalImgHeight = kImgHeightUnits * maze.unit_pixels;
  const auto kPixelCount = static_cast<size_t>(kFinalImgWidth) *
                           static_cast<size_t>(kFinalImgHeight) *
                           static_cast<size_t>(kRgbChannels);
  return {.img_width_units = kImgWidthUnits,
          .img_height_units = kImgHeightUnits,
          .final_img_width = kFinalImgWidth,
          .final_img_height = kFinalImgHeight,
          .pixel_count = kPixelCount};
}

auto IsValidMazeConfig(const Config::MazeConfig& maze, std::string& error)
    -> bool {
  if (maze.width <= 0 || maze.height <= 0 || maze.unit_pixels <= 0) {
    error =
        "Invalid maze dimensions or unit pixels for saving image. Aborting "
        "save.";
    return false;
  }
  return true;
}

auto IsOuterFrameUnit(UnitCoord unit, const ImageSize& image_size) -> bool {
  return unit.row == 0 || unit.row == image_size.img_height_units - 1 ||
         unit.col == 0 || unit.col == image_size.img_width_units - 1;
}

auto IsCellUnit(UnitCoord unit) -> bool {
  return unit.row % kGridSpacing != 0 && unit.col % kGridSpacing != 0;
}

auto IsVerticalWallUnit(UnitCoord unit) -> bool {
  return unit.row % kGridSpacing != 0 && unit.col % kGridSpacing == 0;
}

auto IsHorizontalWallUnit(UnitCoord unit) -> bool {
  return unit.row % kGridSpacing == 0 && unit.col % kGridSpacing != 0;
}

auto SelectCellColor(MazeCoord cell, const SearchFrame& frame,
                     const Config::MazeConfig& maze,
                     const Config::ColorConfig& colors)
    -> const unsigned char* {
  const SolverCellState kState = frame.visual_states_[cell.row][cell.col];
  const unsigned char* current_color_ptr = colors.background;
  if (kState == SolverCellState::START) {
    current_color_ptr = colors.start;
  } else if (kState == SolverCellState::END) {
    current_color_ptr = colors.end;
  } else if (kState == SolverCellState::SOLUTION) {
    current_color_ptr = colors.solution_path;
  } else if (kState == SolverCellState::CURRENT_PROC) {
    current_color_ptr = colors.current;
  } else if (kState == SolverCellState::FRONTIER) {
    current_color_ptr = colors.frontier;
  } else if (kState == SolverCellState::VISITED_PROC) {
    current_color_ptr = colors.visited;
  }

  if (kState != SolverCellState::SOLUTION) {
    if (cell.row == maze.start_node.first &&
        cell.col == maze.start_node.second &&
        kState != SolverCellState::END) {
      current_color_ptr = colors.start;
    }
    if (cell.row == maze.end_node.first && cell.col == maze.end_node.second &&
        kState != SolverCellState::START) {
      current_color_ptr = colors.end;
    }
  }

  return current_color_ptr;
}

auto SelectVerticalWallColor(UnitCoord unit,
                             const MazeDomain::MazeGrid& maze_ref,
                             const Config::MazeConfig& maze,
                             const Config::ColorConfig& colors)
    -> const unsigned char* {
  const int kMazeRow = (unit.row - 1) / kGridSpacing;
  const int kMazeColLeft = (unit.col / kGridSpacing) - 1;
  if (kMazeRow >= 0 && kMazeRow < maze.height && kMazeColLeft >= 0 &&
      kMazeColLeft < maze.width) {
    if (!maze_ref[kMazeRow][kMazeColLeft].walls[kWallRight]) {
      return colors.background;
    }
    return colors.inner_wall;
  }
  return colors.inner_wall;
}

auto SelectHorizontalWallColor(UnitCoord unit,
                               const MazeDomain::MazeGrid& maze_ref,
                               const Config::MazeConfig& maze,
                               const Config::ColorConfig& colors)
    -> const unsigned char* {
  const int kMazeRowUp = (unit.row / kGridSpacing) - 1;
  const int kMazeCol = (unit.col - 1) / kGridSpacing;
  if (kMazeRowUp >= 0 && kMazeRowUp < maze.height && kMazeCol >= 0 &&
      kMazeCol < maze.width) {
    if (!maze_ref[kMazeRowUp][kMazeCol].walls[kWallBottom]) {
      return colors.background;
    }
    return colors.inner_wall;
  }
  return colors.inner_wall;
}

auto SelectUnitColor(UnitCoord unit, const ImageSize& image_size,
                     const SearchFrame& frame,
                     const MazeDomain::MazeGrid& maze_ref,
                     const Config::MazeConfig& maze,
                     const Config::ColorConfig& colors)
    -> const unsigned char* {
  if (IsOuterFrameUnit(unit, image_size)) {
    return colors.outer_wall;
  }
  if (IsCellUnit(unit)) {
    const MazeCoord kCell{.row = (unit.row - 1) / kGridSpacing,
                          .col = (unit.col - 1) / kGridSpacing};
    return SelectCellColor(kCell, frame, maze, colors);
  }
  if (IsVerticalWallUnit(unit)) {
    return SelectVerticalWallColor(unit, maze_ref, maze, colors);
  }
  if (IsHorizontalWallUnit(unit)) {
    return SelectHorizontalWallColor(unit, maze_ref, maze, colors);
  }
  return colors.inner_wall;
}

void PaintUnitPixels(std::vector<unsigned char>& pixels,
                     const ImageContext& context, UnitCoord unit,
                     const unsigned char* color) {
  for (int py = 0; py < context.unit_pixels; ++py) {
    for (int px = 0; px < context.unit_pixels; ++px) {
      const int kImgX = (unit.col * context.unit_pixels) + px;
      const int kImgY = (unit.row * context.unit_pixels) + py;
      if (kImgX < context.final_img_width && kImgY < context.final_img_height) {
        const auto kPixelIndex =
            static_cast<size_t>((kImgY * context.final_img_width) + kImgX) *
            static_cast<size_t>(kRgbChannels);
        pixels[kPixelIndex] = color[0];
        pixels[kPixelIndex + 1] = color[1];
        pixels[kPixelIndex + 2] = color[2];
      }
    }
  }
}

void PaintCellPixels(std::vector<unsigned char>& pixels,
                     const ImageContext& context, MazeCoord cell,
                     const unsigned char* color) {
  const UnitCoord kUnit{.row = (kGridSpacing * cell.row) + 1,
                        .col = (kGridSpacing * cell.col) + 1};
  PaintUnitPixels(pixels, context, kUnit, color);
}

void PaintBaseImage(std::vector<unsigned char>& pixels,
                    const ImageContext& context, const ImageSize& image_size,
                    const SearchFrame& frame,
                    const MazeDomain::MazeGrid& maze_ref,
                    const Config::MazeConfig& maze,
                    const Config::ColorConfig& colors) {
  for (int row_unit = 0; row_unit < image_size.img_height_units; ++row_unit) {
    for (int col_unit = 0; col_unit < image_size.img_width_units; ++col_unit) {
      const UnitCoord kUnit{.row = row_unit, .col = col_unit};
      const unsigned char* current_color_ptr =
          SelectUnitColor(kUnit, image_size, frame, maze_ref, maze, colors);
      PaintUnitPixels(pixels, context, kUnit, current_color_ptr);
    }
  }
}

void PaintPathPoints(std::vector<unsigned char>& pixels,
                     const ImageContext& context,
                     const std::vector<GridPosition>& path,
                     const Config::MazeConfig& maze,
                     const Config::ColorConfig& colors) {
  for (const auto& point : path) {
    const unsigned char* path_color_ptr = colors.solution_path;
    if (point == maze.start_node) {
      path_color_ptr = colors.start;
    } else if (point == maze.end_node) {
      path_color_ptr = colors.end;
    }
    const MazeCoord kCell{.row = point.first, .col = point.second};
    PaintCellPixels(pixels, context, kCell, path_color_ptr);
  }
}

auto TryGetCorridorUnit(const GridPosition& first,
                        const GridPosition& second)
    -> std::optional<UnitCoord> {
  if (first.first == second.first) {
    const int kRowUnit = (kGridSpacing * first.first) + 1;
    const int kColUnit =
        (kGridSpacing * std::min(first.second, second.second)) + kGridSpacing;
    return UnitCoord{.row = kRowUnit, .col = kColUnit};
  }
  if (first.second == second.second) {
    const int kColUnit = (kGridSpacing * first.second) + 1;
    const int kRowUnit =
        (kGridSpacing * std::min(first.first, second.first)) + kGridSpacing;
    return UnitCoord{.row = kRowUnit, .col = kColUnit};
  }
  return std::nullopt;
}

void PaintPathCorridors(std::vector<unsigned char>& pixels,
                        const ImageContext& context,
                        const std::vector<GridPosition>& path,
                        const Config::ColorConfig& colors) {
  if (path.size() <= 1) {
    return;
  }

  for (size_t index = 0; index + 1 < path.size(); ++index) {
    const GridPosition kFirst = path[index];
    const GridPosition kSecond = path[index + 1];

    const auto kCorridorUnit = TryGetCorridorUnit(kFirst, kSecond);
    if (!kCorridorUnit.has_value()) {
      continue;
    }
    PaintUnitPixels(pixels, context, *kCorridorUnit, colors.solution_path);
  }
}

auto BuildFramePath(const fs::path& folder_path, int step_count) -> fs::path {
  std::stringstream path_stream;
  path_stream << folder_path.string() << "/frame_"
              << std::setw(kFrameIndexWidth) << std::setfill('0') << step_count
              << ".png";
  return {path_stream.str()};
}

auto WritePng(const fs::path& path, int width, int height,
              const std::vector<unsigned char>& pixels, std::string& error)
    -> bool {
  const int kWriteOk =
      stbi_write_png(path.string().c_str(), width, height, kRgbChannels,
                     pixels.data(), width * kRgbChannels);
  if (kWriteOk == 0) {
    error = "Failed to write image: " + path.string();
    return false;
  }
  return true;
}

auto SaveImage(const fs::path& folder_path, int step_count,
               const SearchFrame& frame, const MazeDomain::MazeGrid& maze_ref,
               const Config::AppConfig& config, std::string& error) -> bool {
  const auto& maze = config.maze;
  const auto& colors = config.colors;

  if (!IsValidMazeConfig(maze, error)) {
    return false;
  }

  if (frame.visual_states_.empty()) {
    return true;
  }

  const ImageSize kImageSize = ComputeImageSize(maze);
  const ImageContext kContext = {
      .final_img_width = kImageSize.final_img_width,
      .final_img_height = kImageSize.final_img_height,
      .unit_pixels = maze.unit_pixels};
  std::vector<unsigned char> pixels(kImageSize.pixel_count);

  PaintBaseImage(pixels, kContext, kImageSize, frame, maze_ref, maze, colors);
  PaintPathPoints(pixels, kContext, frame.current_path_, maze, colors);
  PaintPathCorridors(pixels, kContext, frame.current_path_, colors);

  const fs::path kFramePath = BuildFramePath(folder_path, step_count);
  return WritePng(kFramePath, kImageSize.final_img_width,
                  kImageSize.final_img_height, pixels, error);
}

}  // namespace

namespace MazeSolver {

auto RenderSearchResult(const SearchResult& result,
                        const MazeDomain::MazeGrid& maze_ref,
                        SolverAlgorithmType algorithm_type,
                        std::string_view generation_algorithm_name,
                        const Config::AppConfig& config) -> RenderResult {
  RenderResult render_result;
  const auto& maze = config.maze;
  if (maze.width <= 0 || maze.height <= 0 || maze.unit_pixels <= 0) {
    render_result.ok = false;
    render_result.error =
        "Invalid maze dimensions or unit pixels. Aborting render.";
    return render_result;
  }

  if (result.frames_.empty()) {
    render_result.ok = false;
    render_result.error = "No frames to render.";
    return render_result;
  }

  const auto kHeightSize = static_cast<size_t>(maze.height);
  const auto kWidthSize = static_cast<size_t>(maze.width);
  if (maze_ref.size() != kHeightSize ||
      (!maze_ref.empty() && maze_ref.front().size() != kWidthSize)) {
    render_result.ok = false;
    render_result.error =
        "Maze grid dimensions do not match config. Aborting render.";
    return render_result;
  }

  fs::path base_dir = config.output_dir.empty() ? "." : config.output_dir;
  const std::string kFolderName = SolverFolderPrefix(algorithm_type) +
                                  std::string(generation_algorithm_name);
  fs::path folder_path = base_dir / kFolderName;
  std::error_code fs_error;
  fs::create_directories(folder_path, fs_error);
  if (fs_error) {
    render_result.ok = false;
    render_result.error = "Failed to create output directory '" +
                          folder_path.string() + "': " + fs_error.message();
    return render_result;
  }

  for (size_t frame_index = 0; frame_index < result.frames_.size();
       ++frame_index) {
    std::string save_error;
    if (!SaveImage(folder_path, static_cast<int>(frame_index),
                   result.frames_[frame_index], maze_ref, config, save_error)) {
      render_result.ok = false;
      render_result.error = save_error;
      return render_result;
    }
    render_result.frames_written++;
  }

  render_result.output_folder = folder_path.string();
  return render_result;
}

}  // namespace MazeSolver
