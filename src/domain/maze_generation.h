#ifndef MAZE_DOMAIN_MAZE_GENERATION_H
#define MAZE_DOMAIN_MAZE_GENERATION_H

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "domain/maze_grid.h"

namespace MazeDomain {

// Enum to specify the maze generation algorithm
enum class MazeAlgorithmType {
  DFS,                 // Recursive Backtracker (Randomized DFS)
  PRIMS,               // Randomized Prim's Algorithm
  KRUSKAL,             // Randomized Kruskal's Algorithm
  RECURSIVE_DIVISION,  // Recursive Division
  GROWING_TREE         // Growing Tree
};

enum class Direction { Up = 0, Right = 1, Down = 2, Left = 3 };

class MazeGeneratorFactory {
 public:
  using Generator = std::function<void(MazeGrid&, int start_r, int start_c,
                                       int grid_width, int grid_height)>;

  static MazeGeneratorFactory& instance();

  void register_generator(MazeAlgorithmType type, std::string name,
                          Generator generator);
  bool has_generator(MazeAlgorithmType type) const;
  Generator get_generator(MazeAlgorithmType type) const;
  std::string name_for(MazeAlgorithmType type) const;
  bool try_parse(std::string_view name, MazeAlgorithmType& out_type) const;
  std::vector<std::string> names() const;

 private:
  MazeGeneratorFactory();
  struct Entry {
    std::string name;
    Generator generator;
  };

  std::map<MazeAlgorithmType, Entry> registry_;
  std::map<std::string, MazeAlgorithmType> name_to_type_;
};

// Pure domain behavior: generates maze structure into the provided grid.
void generate_maze_structure(MazeGrid& maze_grid_to_populate, int start_r,
                             int start_c, int grid_width, int grid_height,
                             MazeAlgorithmType algorithm_type);

// Factory-backed metadata helpers.
std::string algorithm_name(MazeAlgorithmType algorithm_type);
bool try_parse_algorithm(std::string_view name, MazeAlgorithmType& out_type);
std::vector<std::string> supported_algorithms();

}  // namespace MazeDomain

#endif  // MAZE_DOMAIN_MAZE_GENERATION_H
