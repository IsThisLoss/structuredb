#include "hints.hpp"

namespace structuredb::cli {

namespace {

std::string kNoHints = "";

enum class Color {
  kGray = 30,
  kRed = 31,
  kGreen = 32,
  kYellow = 33,
  kBlue = 34,
  kMagenta = 35,
  kCyan = 36,
  kWhite = 37,
};

}

Hints& Hints::Add(const std::string& key, const std::string& value) {
  hints_[key] = value;
  return *this;
}

std::string& Hints::GetHint(const std::string& key) {
  auto it = hints_.find(key);
  if (it != hints_.end()) {
    return it->second;
  }
  return kNoHints;
}

} // namespace structuredb::cli
