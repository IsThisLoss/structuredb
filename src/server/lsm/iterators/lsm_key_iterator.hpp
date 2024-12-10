#pragma once

#include <map>

#include <lsm/lsm.hpp>

namespace structuredb::server::lsm {

class LsmKeyIterator : public Iterator {
public:
  static Awaitable<LsmKeyIterator> Create(Lsm* lsm, std::string key);

  bool HasMore() const override;

  Awaitable<Record> Next() override;
private:
  explicit LsmKeyIterator(Lsm* lsm, std::string key);

  Lsm* lsm_;
  const ScanRange range_;

  bool mem_table_checked_{false};
  int64_t ro_mem_table_idx_{0};
  int64_t ss_table_idx_{0};

  Iterator::Ptr current_iterator_{nullptr};

  Awaitable<Iterator::Ptr> GetNextIterator();

  Awaitable<void> Advance();
};

}


