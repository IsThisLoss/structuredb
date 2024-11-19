#include "logged_table.hpp"

#include <iostream>

#include <wal/events/logged_table_upsert_event.hpp>

namespace structuredb::server::table {

LoggedTable::LoggedTable(io::Manager& io_manager, const std::string& base_dir, const std::string& table_name)
  : lsm_{io_manager, base_dir}, table_name_{table_name}
{}

Awaitable<void> LoggedTable::Init() {
  co_await lsm_.Init();
}

void LoggedTable::StartLogInto(wal::Writer::Ptr wal_writer) {
  wal_writer_ = std::move(wal_writer);
  std::cerr << "Start wal\n";
}

Awaitable<void> LoggedTable::RecoverFromLog(const lsm::Sequence seq_no, const std::string& key, const std::string& value) {
  const bool is_restored = co_await lsm_.Put(seq_no, key, value);
  std::cerr << "Restored: " << seq_no << " " << (is_restored ? "APPLIED" : "SKIPPED") << std::endl;
}

Awaitable<void> LoggedTable::Upsert(const std::string& key, const std::string& value) {
  const auto seq_no = co_await lsm_.Put(key, value);

  if (wal_writer_) {
    co_await wal_writer_->Write(std::make_unique<wal::LoggedTableUpsertEvent>(table_name_, seq_no, key, value));
  }
}

Awaitable<std::optional<std::string>> LoggedTable::Get(const std::string& key) {
  const auto result = co_await lsm_.Get(key);
  co_return result;
}

Awaitable<void> LoggedTable::Scan(const std::string& key, const lsm::RecordConsumer& consume) {
  co_await lsm_.Scan(key, consume);
}

}

