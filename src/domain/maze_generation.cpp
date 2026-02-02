#include "domain/maze_generation.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

namespace {

using Grid = MazeDomain::MazeGrid;
using Direction = MazeDomain::Direction;

struct DirectionInfo {
  int dr;
  int dc;
  Direction opposite;
};

struct GridSize {
  int width;
  int height;
};

constexpr std::array<DirectionInfo, 4> kDirectionTable = {{
    {.dr = -1, .dc = 0, .opposite = Direction::Down},   // Up
    {.dr = 0, .dc = 1, .opposite = Direction::Left},    // Right
    {.dr = 1, .dc = 0, .opposite = Direction::Up},      // Down
    {.dr = 0, .dc = -1, .opposite = Direction::Right},  // Left
}};

constexpr std::array<Direction, 4> kAllDirections = {
    Direction::Up, Direction::Right, Direction::Down, Direction::Left};

auto DirectionIndex(Direction dir) -> int {
  return static_cast<int>(dir);
}

void Carve(Grid& grid, int row, int col, Direction dir) {
  const auto kInfo = kDirectionTable[DirectionIndex(dir)];
  int next_row = row + kInfo.dr;
  int next_col = col + kInfo.dc;
  grid[row][col].walls[DirectionIndex(dir)] = false;
  grid[next_row][next_col].walls[DirectionIndex(kInfo.opposite)] = false;
}

auto CreateVisitedGrid(GridSize grid_size)
    -> std::vector<std::vector<bool>> {
  const auto kHeightSize = static_cast<size_t>(grid_size.height);
  const auto kWidthSize = static_cast<size_t>(grid_size.width);
  return std::vector<std::vector<bool>>(kHeightSize,
                                        std::vector<bool>(kWidthSize, false));
}

void GenerateMazeRecursiveInternal(int row, int col, Grid& current_maze_data,
                                   std::vector<std::vector<bool>>& visited,
                                   int width, int height) {
  visited[row][col] = true;
  std::vector<Direction> directions = {Direction::Up, Direction::Right,
                                       Direction::Down, Direction::Left};

  std::random_device random_device;
  std::mt19937 engine(random_device());
  std::shuffle(directions.begin(), directions.end(), engine);

  for (Direction dir : directions) {
    const auto kInfo = kDirectionTable[DirectionIndex(dir)];
    int next_row = row + kInfo.dr;
    int next_col = col + kInfo.dc;

    if (next_row >= 0 && next_row < height && next_col >= 0 &&
        next_col < width && !visited[next_row][next_col]) {
      Carve(current_maze_data, row, col, dir);
      GenerateMazeRecursiveInternal(next_row, next_col, current_maze_data,
                                    visited, width, height);
    }
  }
}

struct FrontierEdge {
  int row1;
  int col1;
  int row2;
  int col2;
  Direction dir_from_r1;  // direction from (row1,col1) to (row2,col2)
};

void GenerateMazePrimsInternal(int start_row, int start_col,
                               Grid& current_maze_data, int width, int height) {
  const GridSize kGridSize{.width = width, .height = height};
  auto visited = CreateVisitedGrid(kGridSize);
  for (int row_index = 0; row_index < height; ++row_index) {
    for (int col_index = 0; col_index < width; ++col_index) {
      for (bool& wall : current_maze_data[row_index][col_index].walls) {
        wall = true;
      }
    }
  }

  std::random_device random_device;
  std::mt19937 engine(random_device());

  visited[start_row][start_col] = true;
  std::vector<FrontierEdge> frontier_walls;

  for (Direction dir : kAllDirections) {
    const auto kInfo = kDirectionTable[DirectionIndex(dir)];
    int next_row = start_row + kInfo.dr;
    int next_col = start_col + kInfo.dc;
    if (next_row >= 0 && next_row < height && next_col >= 0 &&
        next_col < width) {
      frontier_walls.push_back(
          {start_row, start_col, next_row, next_col, dir});
    }
  }

  while (!frontier_walls.empty()) {
    std::uniform_int_distribution<> distrib(
        0, static_cast<int>(frontier_walls.size()) - 1);
    int random_index = distrib(engine);
    FrontierEdge frontier_edge = frontier_walls[random_index];

    frontier_walls[random_index] = frontier_walls.back();
    frontier_walls.pop_back();

    if (!visited[frontier_edge.row2][frontier_edge.col2]) {
      Carve(current_maze_data, frontier_edge.row1, frontier_edge.col1,
            frontier_edge.dir_from_r1);
      visited[frontier_edge.row2][frontier_edge.col2] = true;

      for (Direction dir : kAllDirections) {
        const auto kInfo = kDirectionTable[DirectionIndex(dir)];
        int next_row = frontier_edge.row2 + kInfo.dr;
        int next_col = frontier_edge.col2 + kInfo.dc;

        if (next_row >= 0 && next_row < height && next_col >= 0 &&
            next_col < width && !visited[next_row][next_col]) {
          frontier_walls.push_back(
              {frontier_edge.row2, frontier_edge.col2, next_row, next_col, dir});
        }
      }
    }
  }
}

struct WallEdge {
  int row1;
  int col1;
  int row2;
  int col2;
  Direction dir_from_r1;
};

class DSU {
 public:
  explicit DSU(int n) : parent_(n), rank_(n, 0) {
    std::ranges::iota(parent_, 0);
  }

  auto Find(int index) -> int {
    if (parent_[index] == index) {
      return index;
    }
    return parent_[index] = Find(parent_[index]);
  }

  void Unite(int left_index, int right_index) {
    int root_i = Find(left_index);
    int root_j = Find(right_index);
    if (root_i != root_j) {
      if (rank_[root_i] < rank_[root_j]) {
        std::swap(root_i, root_j);
      }
      parent_[root_j] = root_i;
      if (rank_[root_i] == rank_[root_j]) {
        rank_[root_i]++;
      }
    }
  }

 private:
  std::vector<int> parent_;
  std::vector<int> rank_;
};

void GenerateMazeKruskalInternal(Grid& current_maze_data, int width,
                                 int height) {
  for (int row_index = 0; row_index < height; ++row_index) {
    for (int col_index = 0; col_index < width; ++col_index) {
      for (bool& wall : current_maze_data[row_index][col_index].walls) {
        wall = true;
      }
    }
  }

  std::vector<WallEdge> all_walls;
  for (int row = 0; row < height; ++row) {
    for (int col = 0; col < width; ++col) {
      if (col < width - 1) {
        all_walls.push_back({row, col, row, col + 1, Direction::Right});
      }
      if (row < height - 1) {
        all_walls.push_back({row, col, row + 1, col, Direction::Down});
      }
    }
  }

  std::random_device random_device;
  std::mt19937 engine(random_device());
  std::shuffle(all_walls.begin(), all_walls.end(), engine);

  DSU dsu(width * height);
  int num_edges_added = 0;
  int total_cells = width * height;

  for (const auto& wall_edge : all_walls) {
    if (num_edges_added >= total_cells - 1 && total_cells > 0) {
      break;
    }

    int cell1_index = (wall_edge.row1 * width) + wall_edge.col1;
    int cell2_index = (wall_edge.row2 * width) + wall_edge.col2;

    if (dsu.Find(cell1_index) != dsu.Find(cell2_index)) {
      Carve(current_maze_data, wall_edge.row1, wall_edge.col1,
            wall_edge.dir_from_r1);
      dsu.Unite(cell1_index, cell2_index);
      num_edges_added++;
    }
  }
}

void GenerateMazeDfs(Grid& maze, int start_row, int start_col, int width,
                     int height) {
  const GridSize kGridSize{.width = width, .height = height};
  auto visited = CreateVisitedGrid(kGridSize);
  GenerateMazeRecursiveInternal(start_row, start_col, maze, visited, width,
                                height);
}

void GenerateMazePrims(Grid& maze, int start_row, int start_col, int width,
                       int height) {
  GenerateMazePrimsInternal(start_row, start_col, maze, width, height);
}

void GenerateMazeKruskal(Grid& maze, int /*start_row*/, int /*start_col*/,
                         int width, int height) {
  GenerateMazeKruskalInternal(maze, width, height);
}

}  // namespace

namespace MazeDomain {

MazeGeneratorFactory::MazeGeneratorFactory() {
  register_generator(MazeAlgorithmType::DFS, "DFS", GenerateMazeDfs);
  register_generator(MazeAlgorithmType::PRIMS, "Prims", GenerateMazePrims);
  register_generator(MazeAlgorithmType::KRUSKAL, "Kruskal",
                     GenerateMazeKruskal);
}

auto MazeGeneratorFactory::instance() -> MazeGeneratorFactory& {
  static MazeGeneratorFactory instance;
  return instance;
}

void MazeGeneratorFactory::register_generator(MazeAlgorithmType type,
                                              std::string name,
                                              Generator generator) {
  registry_[type] =
      Entry{.name = std::move(name), .generator = std::move(generator)};

  std::string key;
  key.reserve(registry_[type].name.size());
  for (unsigned char character : registry_[type].name) {
    key.push_back(static_cast<char>(std::toupper(character)));
  }
  name_to_type_[key] = type;
}

auto MazeGeneratorFactory::has_generator(MazeAlgorithmType type) const -> bool {
  return registry_.contains(type);
}

auto MazeGeneratorFactory::get_generator(MazeAlgorithmType type) const
    -> MazeGeneratorFactory::Generator {
  auto iterator = registry_.find(type);
  if (iterator != registry_.end()) {
    return iterator->second.generator;
  }
  return {};
}

auto MazeGeneratorFactory::name_for(MazeAlgorithmType type) const
    -> std::string {
  auto iterator = registry_.find(type);
  if (iterator != registry_.end()) {
    return iterator->second.name;
  }
  return {};
}

auto MazeGeneratorFactory::try_parse(std::string_view name,
                                     MazeAlgorithmType& out_type) const
    -> bool {
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

auto MazeGeneratorFactory::names() const -> std::vector<std::string> {
  std::vector<std::string> result;
  result.reserve(registry_.size());
  for (const auto& entry : registry_) {
    result.push_back(entry.second.name);
  }
  return result;
}

void generate_maze_structure(MazeGrid& maze_grid_to_populate, int start_r,
                             int start_c, int grid_width, int grid_height,
                             MazeAlgorithmType algorithm_type) {
  if (algorithm_type == MazeAlgorithmType::DFS ||
      algorithm_type == MazeAlgorithmType::PRIMS) {
    if (start_r < 0 || start_r >= grid_height || start_c < 0 ||
        start_c >= grid_width) {
      start_r = 0;
      start_c = 0;
    }
  }

  for (int row_index = 0; row_index < grid_height; ++row_index) {
    for (int col_index = 0; col_index < grid_width; ++col_index) {
      for (bool& wall : maze_grid_to_populate[row_index][col_index].walls) {
        wall = true;
      }
    }
  }

  auto generator =
      MazeGeneratorFactory::instance().get_generator(algorithm_type);
  if (!generator) {
    generator =
        MazeGeneratorFactory::instance().get_generator(MazeAlgorithmType::DFS);
  }
  if (generator) {
    generator(maze_grid_to_populate, start_r, start_c, grid_width, grid_height);
  }
}

auto algorithm_name(MazeAlgorithmType algorithm_type) -> std::string {
  return MazeGeneratorFactory::instance().name_for(algorithm_type);
}

auto try_parse_algorithm(std::string_view name, MazeAlgorithmType& out_type)
    -> bool {
  return MazeGeneratorFactory::instance().try_parse(name, out_type);
}

auto supported_algorithms() -> std::vector<std::string> {
  return MazeGeneratorFactory::instance().names();
}

}  // namespace MazeDomain
