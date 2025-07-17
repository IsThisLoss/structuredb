#include "completion.hpp"

namespace structuredb::cli {

Completion& Completion::Add(const std::string& key, const std::string& value) {
  completion_trie_.Add(key);
  hints_.Add(key, value);
  return *this;
}

std::vector<std::string> Completion::FindCompletion(const std::string& prefix) const {
  return completion_trie_.FindCompletion(prefix);
}

std::string& Completion::GetHint(const std::string& key) {
  return hints_.GetHint(key);
}

}
