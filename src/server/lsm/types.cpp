#include "types.hpp"

namespace structuredb::server::lsm {

bool Record::operator<(const Record& rhs) const {
  if (key != rhs.key) {
    return key < rhs.key;
  }
  if (seq_no != rhs.seq_no) {
    return seq_no > rhs.seq_no;
  }
  return value < rhs.value;
}

}

