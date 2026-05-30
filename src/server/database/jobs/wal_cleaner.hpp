#pragma once

#include <memory>
#include <string>
#include <database/jobs/job.hpp>

namespace structuredb::server::database {

class WalCleaner : public Job {
public:
  using Ptr = std::shared_ptr<WalCleaner>;

  explicit WalCleaner(io::Manager& io_manager, Database& database, std::string wal_dir_path, std::chrono::milliseconds interval);

  std::chrono::milliseconds GetInterval() const override;

  Awaitable<void> Step() override;

private:
  io::Manager& io_manager_;
  Database& database_;
  const std::string wal_dir_path_;
  const std::chrono::milliseconds interval_;
};

}
