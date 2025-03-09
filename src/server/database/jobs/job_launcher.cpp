#include "job_launcher.hpp"

#include <boost/asio/steady_timer.hpp>

namespace structuredb::server::database {

JobLauncher::JobLauncher(io::Manager& io_manager)
  : io_manager_{io_manager}
{}

void JobLauncher::Launch(Job::Ptr job) {
  io_manager_.CoSpawn([this, jb = std::move(job)] () -> Awaitable<void> {
      const auto interval = jb->GetInterval();
      boost::asio::steady_timer timer{io_manager_.Context(), interval};
      while (true) {
        try {
          co_await timer.async_wait(boost::asio::use_awaitable);
          co_await jb->Step();
          timer.expires_after(interval);
        } catch (const std::exception& e) {
          SPDLOG_ERROR("Job failed: {}", e.what());
        }
      }
  });
}

}
