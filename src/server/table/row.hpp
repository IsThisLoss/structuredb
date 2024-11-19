#pragma once

#include <transaction/types.hpp>
#include <lsm/types.hpp>

namespace structuredb::server::table {

struct Row {
  transaction::TransactionId tx{};
  lsm::Sequence seq_no{};
  std::string key{};
  std::string value{};
};

}
