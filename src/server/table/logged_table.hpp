#pragma once

#include <io/manager.hpp>
#include <lsm/lsm.hpp>
#include <wal/writer.hpp>

namespace structuredb::server::table {

class LoggedTable {
public:
  using Ptr = std::shared_ptr<LoggedTable>;

  explicit LoggedTable(io::Manager& io_manager, const std::string& base_dir, const std::string& table_name);

  Awaitable<void> Init();

  void StartLogInto(wal::Writer::Ptr wal_writer);

  Awaitable<void> RecoverFromLog(const lsm::Sequence seq_no, const std::string& key, const std::string& value);

  Awaitable<void> Upsert(const std::string& key, const std::string& value);

  Awaitable<std::optional<std::string>> Get(const std::string& key);

  Awaitable<void> Scan(const std::string& key, const lsm::RecordConsumer& consume);
private:
  const std::string table_name_;
  lsm::Lsm lsm_;
  wal::Writer::Ptr wal_writer_{nullptr};
  lsm::Sequence seq_no_{0};
};

}

