#pragma once

#include <vector>
#include <string_view>

namespace structuredb::cli {

std::vector<std::string_view> Split(const std::string_view& str, const char delimiter);

}
