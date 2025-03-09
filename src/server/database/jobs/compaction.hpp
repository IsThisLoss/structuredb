#pragma once

#include <database/context.hpp>

#include <database/jobs/job.hpp>

namespace structuredb::server::database {

class Compaction final : public Job {
public:
  explicit Compaction(Database& database, const std::chrono::milliseconds interval);

  std::chrono::milliseconds GetInterval() const override;

  Awaitable<void> Step() override;
private:
  Database& database_;
  const std::chrono::milliseconds interval_;
};

}
