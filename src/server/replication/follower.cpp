#include "follower.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include <grpcpp/grpcpp.h>
#include <grpcpp/create_channel.h>
#include <spdlog/spdlog.h>

#include <replication_service.grpc.pb.h>

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
/// through the async io::Manager; the (blocking) gRPC stream read stays on the
/// follower thread and dispatches this coroutine onto the io_context.
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

void Follower::Run() {
  SPDLOG_INFO("Replication: following leader at {}", leader_address_);
  auto channel = grpc::CreateChannel(leader_address_, grpc::InsecureChannelCredentials());
  auto stub = ::structuredb::v1::Replication::NewStub(channel);

  while (true) {
    try {
      const int64_t from_segment = io_manager_.RunSync(
          wal::NextSegmentToFetch(io_manager_, wal_dir_path_));
      SPDLOG_INFO("Replication: requesting WAL from segment {}", from_segment);

      grpc::ClientContext context;
      ::structuredb::v1::GetEventsRequest request;
      auto* from = request.mutable_from();
      from->set_segment_no(from_segment);
      from->set_page_no(0);

      auto reader = stub->GetEvents(&context, request);
      ::structuredb::v1::WalPage page;
      while (reader->Read(&page)) {
        const int64_t segment_no = page.position().segment_no();
        io_manager_.RunSync(ProcessPage(io_manager_, db_, wal_dir_path_, segment_no, page.data()));
        SPDLOG_DEBUG("Replication: applied WAL segment {}", segment_no);
      }
      const auto status = reader->Finish();
      SPDLOG_WARN("Replication: stream ended: {}", status.error_message());
    } catch (const std::exception& e) {
      SPDLOG_ERROR("Replication: follower error: {}", e.what());
    }
    std::this_thread::sleep_for(kReconnectBackoff);
  }
}

}
