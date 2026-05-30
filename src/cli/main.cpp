#include <optional>
#include <string>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>

#include <linenoise/linenoise.h>

#include <client.hpp>

#include <command/manager.hpp>
#include <execute/context.hpp>
#include <utils/time.hpp>

constexpr const char* kHistoryFile = ".structuredb_cli_history";
constexpr int kMaxHistorySize = 100;

const std::string kExit = "exit";
const std::string kQuit = "quit";

constexpr static const char* kDefaultPrompt = "structuredb> ";
constexpr static const char* kTxPrompt = "structuredb (tx)> ";


const structuredb::cli::CommandManager::Ptr command_manager = std::make_unique<structuredb::cli::CommandManager>();

ABSL_FLAG(std::string, target, "localhost:50051", "Server address");
ABSL_FLAG(std::optional<std::string>, c, std::nullopt, "Command to execute without entering REPL");

void completion(const char* buf, linenoiseCompletions* lc) {
  std::string input(buf);
  const auto completions = command_manager->FindCompletion(input);
  for (const auto& completion : completions) {
    linenoiseAddCompletion(lc, completion.c_str());
  }
}

char* hints(const char* buf, int* color, int* bold) {
  constexpr static const int kHintColor = 32; // green

  std::string input(buf);
  auto& hint = command_manager->GetHint(input);
  if (hint.empty()) {
    return nullptr;
  }
  *color = kHintColor;
  return hint.data();
}

std::string GetHistoryFilePath() {
  const char* home = getenv("HOME");
  if (!home) {
    return kHistoryFile;
  }
  return std::string(home) + "/" + kHistoryFile;
}

const char* Read(const structuredb::cli::Context& context) {
  return linenoise(context.tx ? kTxPrompt : kDefaultPrompt);
}

int main(int argc, char** argv) {
  const auto args = absl::ParseCommandLine(argc, argv);

  const auto target_str = absl::GetFlag(FLAGS_target);

  std::cerr << "Connecting to DB at " << target_str << std::endl;

  auto context = structuredb::cli::Context{
    .db = structuredb::client::Connect(target_str),
    .tx = nullptr,
  };

  RegisterCommands(*command_manager);

  if (const auto cmd = absl::GetFlag(FLAGS_c); cmd.has_value()) {
    try {
      const auto command = command_manager->ParseCommand(cmd.value());
      structuredb::cli::MeasureTime([&]() {
          command->Execute(context);
      });
    } catch (const std::exception& e) {
      std::cerr << "Error: " << e.what() << std::endl;
      return 1;
    }
    return 0;
  }

  const auto history_file = GetHistoryFilePath();

  linenoiseSetCompletionCallback(completion);
  linenoiseSetHintsCallback(hints);
  linenoiseHistoryLoad(history_file.c_str());
  linenoiseHistorySetMaxLen(kMaxHistorySize);

  while (const char* raw_input = Read(context)) {
    std::string line(raw_input);
    linenoiseFree((void*)raw_input);
    if (line.empty()) {
      continue;
    }

    if (line == kExit || line == kQuit) {
      break;
    }

    linenoiseHistoryAdd(line.c_str());

    try {
      const auto command = command_manager->ParseCommand(line);
      structuredb::cli::MeasureTime([&]() {
          command->Execute(context);
      });
    } catch (const std::exception& e) {
      std::cerr << "Error: " << e.what() << std::endl;
    }
  }

  linenoiseHistorySave(history_file.c_str());

  std::cerr << "Exiting CLI." << std::endl;

  return 0;
}
