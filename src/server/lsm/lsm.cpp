#include "lsm.hpp"

#include <iostream>
#include <filesystem>

namespace structuredb::server::lsm {

Lsm::Lsm(io::Manager& io_manager, const std::string& base_dir)
  : io_manager_{io_manager}
  , base_dir_{base_dir}
{}

Awaitable<void> Lsm::Init() {
  Sequence max_persistent_seq_no{};
  for (const auto & dir_entry : std::filesystem::directory_iterator{base_dir_}) {
    auto file_reader = co_await io_manager_.CreateFileReader(dir_entry.path());
    auto ss_table = co_await SSTable::Create(std::move(file_reader));
    max_persistent_seq_no = std::max(max_persistent_seq_no, ss_table.GetMaxSeqNo());
    ss_tables_.push_back(std::move(ss_table));
  }
  next_seq_no_ = max_persistent_seq_no + 1;
  std::cerr << "SSTables ready! " << next_seq_no_ << "\n";
}

Awaitable<Sequence> Lsm::Put(const std::string& key, const std::string& value) {
  const auto seq_no = next_seq_no_++;
  co_await DoPut(seq_no, key, value);
  co_return seq_no;
}

Awaitable<bool> Lsm::Put(const Sequence seq_no, const std::string& key, const std::string& value) {
  if (seq_no != next_seq_no_) {
    co_return false;
  }
  co_await DoPut(seq_no, key, value);
  next_seq_no_++;
  co_return true;
}

Awaitable<void> Lsm::DoPut(const Sequence seq_no, const std::string& key, const std::string& value) {
  mem_table_.Put(Record{key, seq_no, value});

  if (mem_table_.Size() > kMaxRecordsInMemTable) {
    std::cerr << "Mem table reached max size, freeze it\n";
    ro_mem_tables_.push_back(std::move(mem_table_));
    mem_table_ = MemTable{};
  }

  if (ro_mem_tables_.size() > kMaxRoMemTables) {
    std::cerr << "Ro Mem tables reached max size, flush it\n";
    const auto file_path = base_dir_ + "/" + std::to_string(ss_tables_.size()) + ".sst.sdb";
    auto ss_table = co_await ro_mem_tables_.front().Flush(io_manager_, file_path);
    ss_tables_.push_back(std::move(ss_table));
    ro_mem_tables_.erase(ro_mem_tables_.begin());
  }
}

Awaitable<std::optional<std::string>> Lsm::Get(const std::string& key) {
  std::optional<std::string> result;
  co_await Scan(key, [&result](const auto& value) {
      std::cerr << "S " << value << std::endl;
      result = value;
      return true;
  });
  co_return result;
}

Awaitable<void> Lsm::Scan(const std::string& key, const RecordConsumer& consume) {
  std::cerr << "Lsm scan: " << key << std::endl;
  if (mem_table_.Scan(key, consume)) {
    co_return;
  }

  std::cerr << "Ro scan: " << key << std::endl;

  // TODO use Bloom Filter
  for (auto it = ro_mem_tables_.rbegin(); it != ro_mem_tables_.rend(); ++it) {
    if (it->Scan(key, consume)) {
      co_return;
    }
  }

  std::cerr << "SS scan: " << key << std::endl;

  for (auto it = ss_tables_.rbegin(); it != ss_tables_.rend(); ++it) {
    if (co_await it->Scan(key, consume)) {
      co_return;
    }
  }

  std::cerr << "END scan: " << key << std::endl;
}

}
