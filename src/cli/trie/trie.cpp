#include <string>
#include <vector>

#include "trie.hpp"

namespace structuredb::cli {

Trie& Trie::Add(const std::string& word) {
  Node* current = &root_;
  for (char ch : word) {
    const auto it = current->children.find(ch);
    if (it == current->children.end()) {
      current->children.try_emplace(ch, Node{});
    }
    current = &current->children[ch];
  }
  current->is_end_of_word = true;
  return *this;
}

std::vector<std::string> Trie::FindCompletion(const std::string& prefix) const {
  std::vector<std::string> result;

  const Node* current = &root_;
  for (const char ch : prefix) {
    const auto it = current->children.find(ch);
    if (it == current->children.end()) {
      return result;
    }
    current = &it->second;
  }

  std::string suffix = prefix;
  CollectSuffixes(current, suffix, result);
  return result;
}


void Trie::CollectSuffixes(const Node* node, std::string& suffix, std::vector<std::string>& results) const {
  if (node->is_end_of_word) {
    results.push_back(suffix);
  }
  for (const auto& [ch, child] : node->children) {
    suffix.push_back(ch);
    CollectSuffixes(&child, suffix, results);
    suffix.pop_back();
  }
}

}

