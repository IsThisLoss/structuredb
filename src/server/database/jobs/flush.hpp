#pragma once

#include <chrono>
#include <database/context.hpp>

#include <database/jobs/job.hpp>

namespace structuredb::server::database {

class Database;

/// @brief periodically flushes frozen mem tables of every table to disk
class Flush final : public Job {
public:
  explicit Flush(Database& database, const std::chrono::milliseconds interval);

  std::chrono::milliseconds GetInterval() const override;

  Awaitable<void> Step() override;
private:
  Database& database_;
  const std::chrono::milliseconds interval_;
};

}
