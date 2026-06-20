#include "follower.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
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
#include <wal/segment.hpp>

namespace fs = std::filesystem;

namespace structuredb::server::replication {

namespace {

constexpr auto kReconnectBackoff = std::chrono::seconds{1};

/// @brief first WAL segment the follower still needs from the leader
///
/// Empty segments (the trailing one the local WAL writer pre-creates) are
/// ignored, so streaming resumes exactly after the last mirrored page.
int64_t DetectResumeSegment(const std::string& wal_dir) {
  std::error_code ec;
  if (!fs::exists(wal_dir, ec)) {
    return 0;
  }
  int64_t next = 0;
  for (const auto& entry : fs::directory_iterator(wal_dir, ec)) {
    if (!entry.is_regular_file() || entry.file_size(ec) == 0) {
      continue;
    }
    const auto seg = wal::GetSegmentNoFromName(entry.path().filename().string());
    if (seg >= 0) {
      next = std::max(next, seg + 1);
    }
  }
  return next;
}

void MirrorPage(const std::string& wal_dir, int64_t segment_no, const std::string& data) {
  const auto path = std::format("{}/{:04d}.wal.sdb", wal_dir, segment_no);
  std::ofstream out{path, std::ios::binary | std::ios::trunc};
  out.write(data.data(), static_cast<std::streamsize>(data.size()));
}

Awaitable<void> ApplyPageEvents(database::Database& db, std::string data) {
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
      const int64_t from_segment = DetectResumeSegment(wal_dir_path_);
      SPDLOG_INFO("Replication: requesting WAL from segment {}", from_segment);

      grpc::ClientContext context;
      ::structuredb::v1::GetEventsRequest request;
      request.mutable_from()->set_segment_no(from_segment);
      request.mutable_from()->set_page_no(0);

      auto reader = stub->GetEvents(&context, request);
      ::structuredb::v1::WalPage page;
      while (reader->Read(&page)) {
        const int64_t segment_no = page.position().segment_no();
        MirrorPage(wal_dir_path_, segment_no, page.data());
        io_manager_.RunSync(ApplyPageEvents(db_, page.data()));
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
