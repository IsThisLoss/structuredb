#pragma once

#include <chrono>
#include <memory>
#include <string>

#include <replication_service.grpc.pb.h>

#include <io/manager.hpp>

namespace structuredb::server::services {

/// @brief leader-side replication endpoint
///
/// Streams stable WAL pages to followers starting from a requested position.
/// Pages are produced by polling the WAL directory; only completed segments
/// are shipped (see wal::CollectStablePages).
class ReplicationServiceImpl : public ::structuredb::v1::Replication::CallbackService {
public:
  explicit ReplicationServiceImpl(
      io::Manager& io_manager,
      std::string wal_dir_path,
      std::chrono::milliseconds poll_interval
  );

  grpc::ServerWriteReactor<::structuredb::v1::WalPage>* GetEvents(
      grpc::CallbackServerContext* context,
      const ::structuredb::v1::GetEventsRequest* request
  ) override;

private:
  io::Manager& io_manager_;
  const std::string wal_dir_path_;
  const std::chrono::milliseconds poll_interval_;
};

std::unique_ptr<grpc::Service> MakeReplicationService(
    io::Manager& io_manager,
    std::string wal_dir_path,
    std::chrono::milliseconds poll_interval
);

}
