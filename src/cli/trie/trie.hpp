#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace structuredb::cli {

class Trie {
public:
  Trie& Add(const std::string& word);

  std::vector<std::string> FindCompletion(const std::string& prefix) const;

private:
  struct Node {
    bool is_end_of_word = false;
    std::unordered_map<char, Node> children{};
  };

  Node root_;

  void CollectSuffixes(const Node* node, std::string& suffix, std::vector<std::string>& results) const;
};

}
