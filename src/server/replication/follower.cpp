#include "follower.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <boost/asio/post.hpp>

#include <grpcpp/grpcpp.h>
#include <grpcpp/create_channel.h>
#include <spdlog/spdlog.h>

#include <replication_service.grpc.pb.h>

#include <io/condition_variable.hpp>
#include <io/sleep.hpp>
#include <sdb/buffer_reader.hpp>
#include <wal/events/event.hpp>
#include <wal/events/io.hpp>
#include <wal/tail.hpp>

namespace structuredb::server::replication {

namespace {

constexpr auto kReconnectBackoff = std::chrono::seconds{1};

/// @brief mirrors a received page into the local WAL and applies its events
///
/// Mirroring first keeps the follower's durability identical to a standalone
/// node: a crash replays the page through normal recovery. All file I/O goes
/// through the async io::Manager on the server io_context.
Awaitable<void> ProcessPage(
    io::Manager& io_manager,
    database::Database& db,
    const std::string& wal_dir_path,
    int64_t segment_no,
    std::string data
) {
  co_await wal::WriteReceivedPage(io_manager, wal_dir_path, segment_no, data);

  sdb::BufferReader reader{std::vector<char>{data.begin(), data.end()}};
  while (reader.HasMore()) {
    wal::Event::Ptr event;
    try {
      event = wal::ParseEvent(reader);
    } catch (const std::exception&) {
      // reached the zero padding at the tail of the page
      break;
    }
    co_await event->Apply(db);
  }
}

}

Follower::Follower(
    io::Manager& io_manager,
    database::Database& db,
    std::string leader_address,
    std::string wal_dir_path
)
  : io_manager_{io_manager}
  , db_{db}
  , leader_address_{std::move(leader_address)}
  , wal_dir_path_{std::move(wal_dir_path)}
{}

namespace {

// gRPC callbacks run on its threads; state is only accessed on the io_context.
// The hold keeps OnDone from running while a page is being applied and the
// coroutine is deciding whether to start the next read.
class FollowerStream final : public grpc::ClientReadReactor<::structuredb::v1::WalPage>,
                             public std::enable_shared_from_this<FollowerStream> {
public:
  explicit FollowerStream(io::Manager& io_manager)
    : io_manager_{io_manager}, changed_{io_manager.Context()} {}

  void Start(::structuredb::v1::Replication::Stub& stub, int64_t from_segment) {
    request_.mutable_from()->set_segment_no(from_segment);
    request_.mutable_from()->set_page_no(0);
    stub.async()->GetEvents(&context_, &request_, this);
    AddHold();
    StartRead(&page_);
    StartCall();
  }

  void OnReadDone(bool ok) override {
    boost::asio::post(io_manager_.Context(), [self = shared_from_this(), ok]() {
      self->read_ok_ = ok;
      self->read_done_ = true;
      self->changed_.NotifyAll();
    });
  }

  void OnDone(const grpc::Status& status) override {
    boost::asio::post(io_manager_.Context(), [self = shared_from_this(), status]() {
      self->status_ = status;
      self->done_ = true;
      self->changed_.NotifyAll();
    });
  }

  Awaitable<void> Consume(database::Database& db, const std::string& wal_dir_path) {
    try {
      while (true) {
        co_await changed_.Wait([this]() { return read_done_; });
        if (!read_ok_) {
          break;
        }
        co_await ProcessPage(io_manager_, db, wal_dir_path, page_.position().segment_no(), page_.data());
        read_done_ = false;
        StartRead(&page_);
      }
    } catch (const std::exception& e) {
      SPDLOG_ERROR("Replication: follower error: {}", e.what());
      context_.TryCancel();
    }
    RemoveHold();
    co_await changed_.Wait([this]() { return done_; });
    SPDLOG_WARN("Replication: stream ended: {}", status_.error_message());
  }

private:
  io::Manager& io_manager_;
  io::ConditionVariable changed_;
  grpc::ClientContext context_;
  ::structuredb::v1::GetEventsRequest request_;
  ::structuredb::v1::WalPage page_;
  grpc::Status status_;
  bool read_done_{false};
  bool read_ok_{false};
  bool done_{false};
};

}

Awaitable<void> Follower::Run() {
  SPDLOG_INFO("Replication: following leader at {}", leader_address_);
  auto channel = grpc::CreateChannel(leader_address_, grpc::InsecureChannelCredentials());
  auto stub = ::structuredb::v1::Replication::NewStub(channel);

  while (true) {
    try {
      const int64_t from_segment = co_await wal::NextSegmentToFetch(io_manager_, wal_dir_path_);
      SPDLOG_INFO("Replication: requesting WAL from segment {}", from_segment);
      auto stream = std::make_shared<FollowerStream>(io_manager_);
      stream->Start(*stub, from_segment);
      co_await stream->Consume(db_, wal_dir_path_);
    } catch (const std::exception& e) {
      SPDLOG_ERROR("Replication: follower error: {}", e.what());
    }
    co_await io::SleepFor(kReconnectBackoff);
  }
}

}
