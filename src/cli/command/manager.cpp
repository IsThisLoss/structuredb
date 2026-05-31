#include "manager.hpp"

#include <string>
#include <vector>
#include <stdexcept>
#include <utils/split.hpp>

namespace structuredb::cli {

namespace {

std::string kNoHint = "";

}

CommandManager& CommandManager::Add(std::string cmd, Command::Factory factory, std::string hint) {
  completion_trie_.Add(cmd);
  command_factories_.try_emplace(cmd, std::move(factory));
  hints_.try_emplace(std::move(cmd), std::move(hint));
  return *this;
}

std::vector<std::string> CommandManager::FindCompletion(const std::string& prefix) const {
  return completion_trie_.FindCompletion(prefix);
}

std::string& CommandManager::GetHint(const std::string& key) {
  auto it = hints_.find(key);
  if (it != hints_.end()) {
    return it->second;
  }
  return kNoHint;
}

Command::Ptr CommandManager::ParseCommand(const std::string& line) const {
  const auto tokens = Split(line, ' ');
  if (tokens.empty()) {
    throw std::runtime_error("Empty command.");
  }
  const auto& cmd = tokens[0];
  auto it = command_factories_.find(cmd);
  if (it == command_factories_.end()) {
    throw std::runtime_error("Unknown command: " + line);
  }
  auto command = it->second(tokens);
  if (!command) {
    throw std::runtime_error("Unknown command: " + line);
  }
  return command;
}

}
