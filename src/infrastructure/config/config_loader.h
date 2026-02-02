#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <string>
#include <vector>

#include "config/config.h"

namespace ConfigLoader {

struct LoadResult {
  Config::AppConfig config;
  bool ok = true;
  std::string error;
  std::vector<std::string> warnings;
};

LoadResult load_config(const std::string& filename);

}  // namespace ConfigLoader

#endif  // CONFIG_LOADER_H
