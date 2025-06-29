#include <iostream>
#include <memory>
#include <string>

#include <client.hpp>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>


ABSL_FLAG(std::string, target, "localhost:50051", "Server address");
ABSL_FLAG(std::optional<std::string>, tx, std::nullopt, "Transaction number");


int main(int argc, char** argv) {
  const auto args = absl::ParseCommandLine(argc, argv);
  if (args.size() <= 1) {
    std::cerr << "Wrong usage\n";
    return 1;
  }

  const auto target_str = absl::GetFlag(FLAGS_target);
  const auto tx = absl::GetFlag(FLAGS_tx);
  auto client = structuredb::client::Connect(target_str);

  const auto cmd = std::string(args[1]);
  if (cmd == "UPSERT" && args.size() == 5) {
    client->Table(args[2])->Upsert(args[3], args[4]);
    return 0;
  }

  if (cmd == "LOOKUP" && args.size() == 4) {
    std::cout << client->Table(args[2])->Lookup(args[3]).value_or("<null>") << std::endl;
  }
  if (cmd == "DELETE" && args.size() == 4) {
    client->Table(args[2])->Delete(args[3]);
    return 0;
  }

  if (cmd == "CREATE" && args.size() == 3) {
    client->CreateTable(args[2]);
    return 0;
  }

  if (cmd == "DROP" && args.size() == 3) {
    client->DropTable(args[2]);
    return 0;
  }

  std::cerr << "Wrong usage\n";
  return 1;
}
