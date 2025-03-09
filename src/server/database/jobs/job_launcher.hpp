#pragma once

#include <io/manager.hpp>

#include "job.hpp"

namespace structuredb::server::database {

class JobLauncher {
public:
  explicit JobLauncher(io::Manager& io_manager);

  void Launch(Job::Ptr job);
private:
  io::Manager& io_manager_;
};

}
