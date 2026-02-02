#ifndef GENERATION_ALGORITHMS_COMMAND_H
#define GENERATION_ALGORITHMS_COMMAND_H

#include "cli/framework/cli_app.h"

namespace Cli {

void RegisterGenerationAlgorithmsCommand(CliApp& app);
int ApplyGenerationAlgorithms(const std::vector<std::string>& args,
                              CommandContext& ctx);
void PrintSupportedGenerationAlgorithms(std::ostream& out);

}  // namespace Cli

#endif  // GENERATION_ALGORITHMS_COMMAND_H
