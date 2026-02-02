#ifndef MAZE_DOMAIN_MAZE_GRID_H
#define MAZE_DOMAIN_MAZE_GRID_H

#include <vector>

namespace MazeDomain {

constexpr int kWallCount = 4;

struct MazeCell {
  bool walls[kWallCount] = {true, true, true, true};
};

using MazeGrid = std::vector<std::vector<MazeCell>>;

}  // namespace MazeDomain

#endif  // MAZE_DOMAIN_MAZE_GRID_H
