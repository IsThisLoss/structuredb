#pragma once

#include <io/manager.hpp>
#include <lsm/lsm.hpp>
#include <wal/writer.hpp>

namespace structuredb::server::table {

class LsmStorage {
public:
  using Ptr = std::shared_ptr<LsmStorage>;
  using Id = std::string;

  explicit LsmStorage(io::Manager& io_manager, const std::string& base_dir, const std::string& id);

  Awaitable<void> Init();

  void StartLogInto(wal::Writer::Ptr wal_writer);

  Awaitable<void> RecoverFromLog(const lsm::Sequence seq_no, const std::string& key, const std::string& value);

  Awaitable<void> Upsert(const std::string& key, const std::string& value);

  Awaitable<std::optional<std::string>> Get(const std::string& key);

  Awaitable<lsm::Iterator::Ptr> Scan(const std::string& key);

  Awaitable<lsm::Iterator::Ptr> Scan(const std::optional<std::string>& lower_bound, const std::optional<std::string>& upper_bound);
private:
  const Id id_;
  lsm::Lsm lsm_;
  wal::Writer::Ptr wal_writer_{nullptr};
  lsm::Sequence seq_no_{0};
};

}
