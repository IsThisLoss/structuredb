#include "replication_service.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <boost/asio/post.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <spdlog/spdlog.h>

#include <io/condition_variable.hpp>
#include <wal/tail.hpp>

namespace structuredb::server::services {

namespace {

/// @brief streams stable WAL pages to a single follower
///
/// A long-lived pump coroutine runs on the io_context: it collects stable
/// pages, writes them one at a time honouring gRPC backpressure, and polls
/// for new pages when caught up. gRPC delivers OnWriteDone/OnCancel on its
/// own threads, so those are hopped onto the io_context before touching state.
///
/// Lifetime: the reactor is destroyed only in OnDone, which gRPC invokes after
/// Finish() completes. Finish() is always called by the pump coroutine itself,
/// so the coroutine never touches the reactor after it may have been deleted.
class GetEventsReactor final : public grpc::ServerWriteReactor<::structuredb::v1::WalPage> {
public:
  GetEventsReactor(
      io::Manager& io_manager,
      std::string wal_dir_path,
      wal::Position from,
      std::chrono::milliseconds poll_interval
  )
    : io_manager_{io_manager}
    , wal_dir_path_{std::move(wal_dir_path)}
    , next_pos_{from}
    , poll_interval_{poll_interval}
    , write_done_cv_{io_manager.Context()}
  {
    io_manager_.CoSpawn([this]() -> Awaitable<void> { co_await Pump(); });
  }

  void OnWriteDone(bool ok) override {
    boost::asio::post(io_manager_.Context(), [this, ok]() {
      write_ok_ = ok;
      write_pending_ = false;
      write_done_cv_.NotifyAll();
    });
  }

  void OnCancel() override {
    boost::asio::post(io_manager_.Context(), [this]() {
      cancelled_ = true;
      write_done_cv_.NotifyAll();
    });
  }

  void OnDone() override {
    delete this;
  }

private:
  Awaitable<void> Pump() {
    while (!cancelled_) {
      std::vector<wal::WalPageData> pages;
      try {
        pages = co_await wal::CollectStablePages(io_manager_, wal_dir_path_, next_pos_);
      } catch (const std::exception& e) {
        SPDLOG_ERROR("Replication: failed to read WAL: {}", e.what());
      }

      for (auto& page : pages) {
        if (cancelled_) {
          break;
        }
        msg_.Clear();
        auto* pos = msg_.mutable_position();
        pos->set_segment_no(page.position.segment_no);
        pos->set_page_no(page.position.page_no);
        msg_.set_data(page.data.data(), page.data.size());
        next_pos_ = wal::Position{
          .segment_no = page.position.segment_no,
          .page_no = page.position.page_no + 1,
        };

        write_pending_ = true;
        StartWrite(&msg_);
        co_await write_done_cv_.Wait([this]() { return !write_pending_ || cancelled_; });

        if (!write_ok_ || cancelled_) {
          Finish(grpc::Status::OK);
          co_return;
        }
      }

      if (cancelled_) {
        break;
      }

      // caught up: wait before polling the WAL again
      boost::asio::steady_timer timer{io_manager_.Context(), poll_interval_};
      try {
        co_await timer.async_wait(boost::asio::use_awaitable);
      } catch (const std::exception&) {
        // ignore (e.g. cancellation): the loop condition re-checks cancelled_
      }
    }

    Finish(grpc::Status::OK);
  }

  io::Manager& io_manager_;
  const std::string wal_dir_path_;
  wal::Position next_pos_;
  const std::chrono::milliseconds poll_interval_;

  ::structuredb::v1::WalPage msg_{};
  io::ConditionVariable write_done_cv_;
  bool write_pending_{false};
  bool write_ok_{true};
  bool cancelled_{false};
};

}

ReplicationServiceImpl::ReplicationServiceImpl(
    io::Manager& io_manager,
    std::string wal_dir_path,
    std::chrono::milliseconds poll_interval
)
  : io_manager_{io_manager}
  , wal_dir_path_{std::move(wal_dir_path)}
  , poll_interval_{poll_interval}
{}

grpc::ServerWriteReactor<::structuredb::v1::WalPage>* ReplicationServiceImpl::GetEvents(
    grpc::CallbackServerContext* /*context*/,
    const ::structuredb::v1::GetEventsRequest* request
) {
  const wal::Position from{
    .segment_no = request->from().segment_no(),
    .page_no = request->from().page_no(),
  };
  SPDLOG_INFO("Replication: follower attached from segment {}, page {}", from.segment_no, from.page_no);
  return new GetEventsReactor{io_manager_, wal_dir_path_, from, poll_interval_};
}

std::unique_ptr<grpc::Service> MakeReplicationService(
    io::Manager& io_manager,
    std::string wal_dir_path,
    std::chrono::milliseconds poll_interval
) {
  return std::make_unique<ReplicationServiceImpl>(io_manager, std::move(wal_dir_path), poll_interval);
}

}
