#include "printer.hpp"

#include <string>
#include <iostream>

namespace structuredb::cli {

Printer::Printer(const std::string& header) {
  const size_t offset = kLine.size() - header.size();
  std::string padding(offset / 2, ' ');
  std::cout << padding << header << padding << std::endl;
}

void Printer::PrintRow(const std::string& key, const std::string& value) const {
    std::cout << kLine << std::endl;
    const size_t payload_length = 2 + key.size() + 3 + value.size() + 1;
    std::string padding = " ";
    if (payload_length < kLine.size()) {
        padding = std::string(kLine.size() - payload_length, ' ');
    }
    std::cout << kSeparator << " " << key << " " << kSeparator << " " << value << padding << kSeparator << std::endl;
}

Printer::~Printer() {
    std::cout << kLine << std::endl;
}

}
