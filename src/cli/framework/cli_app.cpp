#include "cli/framework/cli_app.h"

#include <utility>

#include "application/services/maze_generation.h"
#include "cli/commands/generation_algorithms_command.h"
#include "common/version.hpp"

namespace Cli {

namespace {

auto IsHelpToken(const std::string& token) -> bool {
  return token == "-h" || token == "--help";
}

auto IsVersionToken(const std::string& token) -> bool {
  return token == "-v" || token == "--version";
}

auto IsOutputToken(const std::string& token) -> bool {
  return token == "-o" || token == "--output";
}

auto IsGenerationAlgorithmsToken(const std::string& token) -> bool {
  return token == "--generation-algorithms";
}

struct OptionOutcome {
  bool consumed = false;
  bool handled = false;
  int exit_code = 0;
};

auto HandleImmediateOption(const std::string& token,
                           const CliApp& app,
                           CommandContext& ctx) -> OptionOutcome {
  OptionOutcome outcome;
  if (IsHelpToken(token)) {
    app.print_help(ctx.out);
    outcome.consumed = true;
    outcome.handled = true;
    outcome.exit_code = 0;
    return outcome;
  }
  if (IsVersionToken(token)) {
    ctx.out << MazeCommon::kVersion << "\n";
    outcome.consumed = true;
    outcome.handled = true;
    outcome.exit_code = 0;
    return outcome;
  }
  return outcome;
}

auto ConsumeOptionValue(const std::string& token,
                        int& index,
                        int argc,
                        char** argv,
                        CommandContext& ctx) -> OptionOutcome {
  OptionOutcome outcome;
  if (IsOutputToken(token)) {
    outcome.consumed = true;
    if (index + 1 >= argc) {
      ctx.err << "Missing value for " << token << "\n";
      outcome.handled = true;
      outcome.exit_code = 1;
      return outcome;
    }
    ctx.config.output_dir = argv[++index];
    return outcome;
  }

  if (IsGenerationAlgorithmsToken(token)) {
    outcome.consumed = true;
    if (index + 1 >= argc) {
      ctx.err << "Missing value for " << token << "\n";
      outcome.handled = true;
      outcome.exit_code = 1;
      return outcome;
    }
    const std::string kValue = argv[++index];
    const int kCode = ApplyGenerationAlgorithms({kValue}, ctx);
    if (kCode != 0) {
      PrintSupportedGenerationAlgorithms(ctx.err);
      outcome.handled = true;
      outcome.exit_code = kCode;
      return outcome;
    }
    return outcome;
  }

  return outcome;
}

void AppendCommandOrArg(std::string& command_name,
                        std::vector<std::string>& args,
                        std::string token) {
  if (command_name.empty()) {
    command_name = std::move(token);
    return;
  }
  args.push_back(std::move(token));
}

}  // namespace

void CliApp::register_command(Command command) {
  commands_.push_back(std::move(command));
}

auto CliApp::commands() const -> const std::vector<Command>& {
  return commands_;
}

void CliApp::print_help(std::ostream& out) const {
  out << "Commands:\n";
  for (const auto& command : commands_) {
    out << "  " << command.name;
    if (!command.description.empty()) {
      out << " - " << command.description;
    }
    out << "\n";
  }
  out << "Options:\n";
  out << "  -v, --version        Show version\n";
  out << "  --generation-algorithms <list>\n";
  out << "                      Override GenerationAlgorithms "
         "(comma-separated)\n";
  out << "  -o, --output <dir>   Set output directory\n";
  out << "  -h, --help           Show this help\n";

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

auto CliApp::run(int argc, char** argv, CommandContext& ctx,
                 bool& handled) const -> int {
  handled = false;
  if (argc <= 1) {
    print_help(ctx.out);
    handled = true;
    return 0;
  }

  std::string command_name;
  std::vector<std::string> args;
  for (int index = 1; index < argc; ++index) {
    std::string token = argv[index];
    const OptionOutcome kImmediateOutcome =
        HandleImmediateOption(token, *this, ctx);
    if (kImmediateOutcome.handled) {
      handled = true;
      return kImmediateOutcome.exit_code;
    }
    if (kImmediateOutcome.consumed) {
      continue;
    }
    const OptionOutcome kValueOutcome =
        ConsumeOptionValue(token, index, argc, argv, ctx);
    if (kValueOutcome.consumed) {
      if (kValueOutcome.handled) {
        handled = true;
        return kValueOutcome.exit_code;
      }
      continue;
    }
    AppendCommandOrArg(command_name, args, std::move(token));
  }

  if (command_name.empty()) {
    print_help(ctx.out);
    handled = true;
    return 0;
  }
  for (const auto& command : commands_) {
    if (command.name == command_name) {
      int code = command.handler(args, ctx);
      if (code != 0 || command.exit_after) {
        handled = true;
        return code;
      }
      handled = false;
      return 0;
    }
  }

  handled = true;
  ctx.err << "Unknown command: " << command_name << "\n";
  return 1;
}

}  // namespace Cli
