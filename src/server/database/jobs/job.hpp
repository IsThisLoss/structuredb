#pragma once

#include <database/context.hpp>

namespace structuredb::server::database {

class Job {
public:
  using Ptr = std::shared_ptr<Job>;
  
  virtual std::chrono::milliseconds GetInterval() const = 0;

  virtual Awaitable<void> Step() = 0;

  virtual ~Job() = default;
};

};
