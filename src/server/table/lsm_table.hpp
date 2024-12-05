#pragma once

#include "table.hpp"

namespace structuredb::server::table {

class LsmTable : public Table {
public:
  virtual Awaitable<void> Upsert(const Key& key, const Value& value) = 0;

  virtual Awaitable<bool> Lookup(const Key& key, Value& value) = 0;
};

}

