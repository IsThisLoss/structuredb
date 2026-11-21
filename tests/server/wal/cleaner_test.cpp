#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <wal/cleaner.hpp>
#include <wal/events/lsm_storage_upsert_event.hpp>
#include <wal/writer.hpp>

#include "../database/common.hpp"

namespace structuredb::tests {
namespace {

class WalCleanerTest : public DatabaseTest {
protected:
  server::Awaitable<void> WriteSegment(const std::vector<int64_t>& sequences) {
    auto writer = co_await server::wal::Writer::Open(GetIoManager(), wal_dir_);
    for (const auto seq : sequences) {
      co_await writer->Write(std::make_unique<server::wal::LsmStorageUpsertEvent>(
          "cleaner_test", seq, "key", "value"));
    }
  }

  server::Awaitable<std::vector<std::string>> Segments() {
    auto segments = co_await GetIoManager().ListDirectory(wal_dir_);
    std::ranges::sort(segments);
    co_return segments;
  }

  std::string wal_dir_;
};

TEST_F(WalCleanerTest, RemovesOnlyFullyPersistedSegmentsAndHonorsRetention) {
  DoTest([this]() -> server::Awaitable<void> {
    auto& db = GetDatabase();
    auto storage = co_await db.EnsureStorageForRecover("cleaner_test");
    wal_dir_ = GetTempDir() + "/cleaner_wal";

    // The fixture freezes a memtable after 51 records. Flush those records,
    // then leave the next one in memory so the two persistence states coexist.
    for (int64_t seq = 1; seq <= 51; ++seq) {
      co_await storage->RecoverFromLog(seq, std::to_string(seq), "value");
    }
    co_await db.Flush();
    co_await storage->RecoverFromLog(52, "52", "value");
    const bool persisted = co_await storage->IsPersistent(51);
    const bool pending = co_await storage->IsPersistent(52);
    EXPECT_TRUE(persisted);
    EXPECT_FALSE(pending);

    co_await WriteSegment({1, 51});
    co_await WriteSegment({51, 52});
    co_await WriteSegment({52, 51});
    co_await WriteSegment({51});
    co_await WriteSegment({});
    EXPECT_EQ(co_await Segments(), (std::vector<std::string>{
        "0000.wal.sdb", "0001.wal.sdb", "0002.wal.sdb", "0003.wal.sdb", "0004.wal.sdb"}));

    co_await server::wal::Clean(GetIoManager(), wal_dir_, db, 3);
    EXPECT_EQ(co_await Segments(), (std::vector<std::string>{
        "0001.wal.sdb", "0002.wal.sdb", "0003.wal.sdb", "0004.wal.sdb"}));

    // Removing the follower's retention boundary releases segment 3, while
    // both orderings of mixed persistence still prevent deletion.
    co_await server::wal::Clean(GetIoManager(), wal_dir_, db);
    EXPECT_EQ(co_await Segments(), (std::vector<std::string>{
        "0001.wal.sdb", "0002.wal.sdb", "0004.wal.sdb"}));

    for (int64_t seq = 53; seq <= 102; ++seq) {
      co_await storage->RecoverFromLog(seq, std::to_string(seq), "value");
    }
    co_await db.Flush();
    const bool now_persisted = co_await storage->IsPersistent(52);
    EXPECT_TRUE(now_persisted);
    co_await server::wal::Clean(GetIoManager(), wal_dir_, db);
    EXPECT_EQ(co_await Segments(), (std::vector<std::string>{"0004.wal.sdb"}));
  });
}

}
}
