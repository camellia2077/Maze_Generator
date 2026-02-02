#ifndef SEARCH_ALGORITHMS_COMMAND_H
#define SEARCH_ALGORITHMS_COMMAND_H

#include "cli/framework/cli_app.h"

namespace Cli {

void RegisterSearchAlgorithmsCommand(CliApp& app);
int ApplySearchAlgorithms(const std::vector<std::string>& args,
                          CommandContext& ctx);
void PrintSupportedSearchAlgorithms(std::ostream& out);

}  // namespace Cli

#endif  // SEARCH_ALGORITHMS_COMMAND_H
