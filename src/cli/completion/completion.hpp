#pragma once

#include <trie/trie.hpp>
#include <hints/hints.hpp>

namespace structuredb::cli {

class Completion {
public:
  Completion& Add(const std::string& key, const std::string& value = "");

  std::vector<std::string> FindCompletion(const std::string& prefix) const;

  std::string& GetHint(const std::string& key);

private:
  Trie completion_trie_;
  Hints hints_;
};

}
