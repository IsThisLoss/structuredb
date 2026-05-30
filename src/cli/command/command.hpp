#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <execute/context.hpp>

namespace structuredb::cli {

class CommandManager;

class Command {
public:
  using Ptr = std::unique_ptr<Command>;
  using Factory = std::function<Command::Ptr(const std::vector<std::string>&)>;

  virtual void Execute(Context& context) const = 0; 

  virtual ~Command() = default;
};

void RegisterCommands(CommandManager& manager);

}
