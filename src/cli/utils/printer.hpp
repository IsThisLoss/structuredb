#pragma once

#include <string_view>
#include <string>

namespace structuredb::cli {

class Printer {
public:
  explicit Printer(const std::string& header);

  void PrintRow(const std::string& key, const std::string& value) const;

  ~Printer();
private:
  static constexpr const char kSeparator = '|';
  static constexpr const std::string_view kLine = "+--------------------------------------------------------------+";
};

}
