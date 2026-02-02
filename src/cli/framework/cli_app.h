#ifndef CLI_APP_H
#define CLI_APP_H

#include <functional>
#include <ostream>
#include <string>
#include <vector>

#include "config/config.h"

namespace Cli {

struct CommandContext {
  Config::AppConfig& config;
  std::ostream& out;
  std::ostream& err;
};

using CommandHandler = std::function<int(const std::vector<std::string>& args,
                                         CommandContext& ctx)>;

struct Command {
  std::string name;
  std::string description;
  CommandHandler handler;
  bool exit_after = true;
};

class CliApp {
 public:
  void register_command(Command command);
  int run(int argc, char** argv, CommandContext& ctx, bool& handled) const;
  const std::vector<Command>& commands() const;
  void print_help(std::ostream& out) const;

 private:
  std::vector<Command> commands_;
};

}  // namespace Cli

#endif  // CLI_APP_H
