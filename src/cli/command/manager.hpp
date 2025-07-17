#pragma once

#include <trie/trie.hpp>
#include <command/command.hpp>

namespace structuredb::cli {

class CommandManager {
public:
  using Ptr = std::unique_ptr<CommandManager>;

  CommandManager& Add(std::string cmd, Command::Factory factory, std::string hint = "");

  std::vector<std::string> FindCompletion(const std::string& prefix) const;

  std::string& GetHint(const std::string& cmd);

  Command::Ptr ParseCommand(const std::string& line) const;
private:
  Trie completion_trie_;
  std::unordered_map<std::string, std::string> hints_;
  std::unordered_map<std::string, Command::Factory> command_factories_;
};

}
