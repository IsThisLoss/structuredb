#pragma once

#include <string>

namespace structuredb::server::table {

/// @brief row of table
struct Row {
  std::string key;
  std::string value;
};

}
