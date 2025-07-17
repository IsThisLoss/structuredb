#include <string>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>

#include <linenoise/linenoise.h>

#include <client.hpp>

#include <completion/completion.hpp>
#include <execute/executor.hpp>


ABSL_FLAG(std::string, target, "localhost:50051", "Server address");

const std::unique_ptr<structuredb::cli::Completion> completion_manager = std::make_unique<structuredb::cli::Completion>();


void completion(const char* buf, linenoiseCompletions* lc) {
  std::string input(buf);
  const auto completions = completion_manager->FindCompletion(input);
  for (const auto& completion : completions) {
    linenoiseAddCompletion(lc, completion.c_str());
  }
}

char* hints(const char* buf, int* color, int* bold) {
  std::string input(buf);
  auto& hint = completion_manager->GetHint(input);
  if (hint.empty()) {
    return nullptr;
  }
  return hint.data();
}

int main(int argc, char** argv) {
  const auto args = absl::ParseCommandLine(argc, argv);

  const auto target_str = absl::GetFlag(FLAGS_target);
  auto executor = structuredb::cli::Executor{structuredb::client::Connect(target_str)};

  (*completion_manager)
    .Add("CREATE TABLE", " <table_name>")
    .Add("UPSERT", " <table_name> <key> <value>")
    .Add("LOOKUP", " <table_name> <key>")
    .Add("DELETE", " <table_name> <key>")
    .Add("BEGIN")
    .Add("COMMIT")
    .Add("ROLLBACK");

  linenoiseSetCompletionCallback(completion);
  linenoiseSetHintsCallback(hints);

  while (true) {
    const char* raw_input = linenoise("structuredb> ");
    if (!raw_input) {
      // EOF or error
      break;
    }
    std::string line(raw_input);
    linenoiseFree((void*)raw_input);
    if (line.empty()) {
      continue;
    }

    executor.Execute(line);

    if (line == "exit" || line == "quit") {
      break;
    }
  }
  return 0;
}
