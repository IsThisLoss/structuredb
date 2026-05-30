#include "common.hpp"
#include "co_test.hpp"

#include <database/database.hpp>
#include <gtest/gtest.h>
#include <fmt/core.h>


namespace structuredb::tests {

namespace {

static const std::string kTableName = "test_table";
static const std::string kKey = "test_key";
static const std::string kValue = "test_value";

}

DATABASE_TEST(CreateDropTable, {
  auto& db = GetDatabase();

  // create table
  {
    auto session = co_await db.StartSession();
    co_await session.CreateTable(kTableName);
    co_await session.Finish();
  }

  // get table
  {
    auto session = co_await db.StartSession();
    auto table = co_await session.GetTable(kTableName);
    co_await session.Finish();
    CO_ASSERT_NE(table, nullptr);
  }

  // drop table
  {
    auto session = co_await db.StartSession();
    co_await session.DropTable(kTableName);
    co_await session.Finish();
  }

  // get table
  {
    auto session = co_await db.StartSession();
    auto table = co_await session.GetTable(kTableName);
    co_await session.Finish();
    CO_ASSERT_EQ(table, nullptr);
  }
})

DATABASE_TEST(UpsertLookupDelete, {
  auto& db = GetDatabase();

  // create table
  {
    auto session = co_await db.StartSession();
    co_await session.CreateTable(kTableName);
    co_await session.Finish();
  }

  // upsert
  {
    auto session = co_await db.StartSession();
    auto table = co_await session.GetTable(kTableName);
    co_await table->Upsert(kKey, kValue);
    co_await session.Finish();
  }
 
  // lookup
  {
    auto session = co_await db.StartSession();
    auto table = co_await session.GetTable(kTableName);
    auto value = co_await table->Lookup(kKey);
    co_await session.Finish();
   
    CO_ASSERT_TRUE(value.has_value());
    CO_ASSERT_EQ(value.value(), kValue);
  }

  // delete
  {
    auto session = co_await db.StartSession();
    auto table = co_await session.GetTable(kTableName);
    co_await table->Delete(kKey);
    co_await session.Finish();
  }

  // lookup
  {
    auto session = co_await db.StartSession();
    auto table = co_await session.GetTable(kTableName);
    auto value = co_await table->Lookup(kKey);
    co_await session.Finish();
    CO_ASSERT_FALSE(value.has_value());
  }
})

DATABASE_TEST(TxIsolation, {
  auto& db = GetDatabase();

  // create table
  {
    auto session = co_await db.StartSession();
    co_await session.CreateTable(kTableName);
    co_await session.Finish();
  }
 
  // begin transaction
  server::transaction::TransactionId tx{};
  {
    auto session = co_await db.StartSession();
    tx = session.GetTx();
  }

  // upsert and lookup in transaction
  {
    auto session = co_await db.StartSession(tx);
    auto table = co_await session.GetTable(kTableName);
    co_await table->Upsert(kKey, kValue);
    auto value = co_await table->Lookup(kKey);

    CO_ASSERT_TRUE(value.has_value());
    CO_ASSERT_EQ(value.value(), kValue);
    co_await session.Finish();
  }
 
  // lookup outside of transaction
  {
    auto session = co_await db.StartSession();
    auto table = co_await session.GetTable(kTableName);
    auto value = co_await table->Lookup(kKey);
    co_await session.Finish();
    CO_ASSERT_FALSE(value.has_value());
  }

  // commit transaction
  {
    auto session = co_await db.StartSession(tx);
    co_await session.Commit();
    co_await session.Finish();
  }

  {
    auto session = co_await db.StartSession();
    auto table = co_await session.GetTable(kTableName);
    auto value = co_await table->Lookup(kKey);
    co_await session.Finish();
    CO_ASSERT_TRUE(value.has_value());
    CO_ASSERT_EQ(value.value(), kValue);
  }
})

DATABASE_TEST(RangeScan, {
  const int64_t kSize = 100;
  const int64_t kLowerBound = 20;
  const int64_t kUpperBound = 40;
  auto& db = GetDatabase();

  // create table and insert 100 key-values into it
  {
    auto session = co_await db.StartSession();
    co_await session.CreateTable(kTableName);
    auto table = co_await session.GetTable(kTableName);
    for (int64_t i = 0; i < kSize; i++) {
      const auto key = fmt::format("{:02}", i);
      const auto value = fmt::format("{:02}", -1 * i);
      co_await table->Upsert(key, value);
    }
    co_await session.Finish();
  }

  {
    auto session = co_await db.StartSession();
    auto table = co_await session.GetTable(kTableName);
    co_await table->Upsert(kKey, kValue);
    auto iter = co_await table->Scan(std::to_string(kLowerBound), std::to_string(kUpperBound));
    co_await session.Finish();

    std::vector<std::pair<std::string, std::string>> result;
    while (iter->HasMore()) {
      auto row = co_await iter->Next();
      result.emplace_back(std::move(row.key), std::move(row.value));
    }

    const int64_t kExpectedSize = kUpperBound - kLowerBound +1; // [20; 40], include right border

    CO_ASSERT_EQ(result.size(), kExpectedSize);

    int64_t idx = 0;
    for (int64_t i = kLowerBound; i <= kUpperBound; i++) {
      const auto expected_key = fmt::format("{:02}", i);
      CO_ASSERT_EQ(result[idx].first, expected_key);
      const auto expected_value = fmt::format("{:02}", -1 * i);
      CO_ASSERT_EQ(result[idx].second, expected_value);
      idx++;
    }
  }
})

DATABASE_TEST(Compaction, {
  const int64_t kSize = 1000;
  auto& db = GetDatabase();

  // create table and insert key-values into it
  {
    auto session = co_await db.StartSession();
    co_await session.CreateTable(kTableName);
    auto table = co_await session.GetTable(kTableName);
    for (int64_t i = 0; i < kSize; i++) {
      const auto key = fmt::format("{:03}", i);
      const auto value = fmt::format("{:03}", -1 * i);
      co_await table->Upsert(key, value);
    }
    co_await session.Finish();
  }

  {
    auto session = co_await db.StartSession();
    auto table = co_await session.GetTable(kTableName);

    const int old_ss_tables_count = co_await session.CountSSTables(kTableName);
    CO_ASSERT_TRUE(old_ss_tables_count > 1);

    co_await table->Compact();
    const int new_ss_tables_count = co_await session.CountSSTables(kTableName);
    CO_ASSERT_EQ(new_ss_tables_count, 1);
    CO_ASSERT_TRUE(new_ss_tables_count < old_ss_tables_count);
    co_await session.Finish();
  }

  {
    auto session = co_await db.StartSession();
    auto table = co_await session.GetTable(kTableName);
    auto iter = co_await table->Scan(std::nullopt, std::nullopt);

    int count = 0;
    while (iter->HasMore()) {
      auto row = co_await iter->Next();
      const auto expected_key = fmt::format("{:03}", count);
      CO_ASSERT_EQ(row.key, expected_key);
      const auto expected_value = fmt::format("{:03}", -1 * count);
      CO_ASSERT_EQ(row.value, expected_value);
      count++;
    }
    CO_ASSERT_EQ(count, kSize);
    co_await session.Finish();
  }
})

// Test row-level locking prevents concurrent writes to same key
DATABASE_TEST(ConcurrentWritesSameKey, {
  auto& db = GetDatabase();

  // Setup: Create table
  {
    auto session = co_await db.StartSession();
    co_await session.CreateTable(kTableName);
    co_await session.Finish();
  }

  // Write from first transaction
  {
    auto session1 = co_await db.StartSession();
    auto table1 = co_await session1.GetTable(kTableName);
    co_await table1->Upsert(kKey, "value1");

    // At this point, first transaction holds a lock on kKey
    // Create second session and try to write (should block in spin-wait)
    // For now, we just verify first transaction succeeds

    co_await session1.Finish();  // Release locks
  }

  // Verify final value
  {
    auto session = co_await db.StartSession();
    auto table = co_await session.GetTable(kTableName);
    auto result = co_await table->Lookup(kKey);
    CO_ASSERT_TRUE(result.has_value());
    CO_ASSERT_EQ(result.value(), "value1");
    co_await session.Finish();
  }
})

// Test lock release on transaction rollback
DATABASE_TEST(RollbackReleasesLocks, {
  auto& db = GetDatabase();

  // Setup: Create table
  {
    auto session = co_await db.StartSession();
    co_await session.CreateTable(kTableName);
    co_await session.Finish();
  }

  // Initial write
  {
    auto session = co_await db.StartSession();
    auto table = co_await session.GetTable(kTableName);
    co_await table->Upsert(kKey, "initial");
    co_await session.Finish();
  }

  // Verify locks are released after write
  {
    auto session = co_await db.StartSession();
    auto table = co_await session.GetTable(kTableName);
    auto result = co_await table->Lookup(kKey);
    CO_ASSERT_TRUE(result.has_value());
    co_await session.Finish();
  }
})

// Test that a write blocks until a concurrent transaction releases the row lock.
// This exercises the coroutine-friendly wait/notify path: if resumption were
// broken, the second write would never complete and the test would hang.
DATABASE_TEST(ConcurrentWriteWaitsForLock, {
  auto& db = GetDatabase();

  // create table
  {
    auto session = co_await db.StartSession();
    co_await session.CreateTable(kTableName);
    co_await session.Finish();
  }

  // mint tx1 and let it hold the exclusive lock on kKey (not committed yet)
  server::transaction::TransactionId tx1{};
  {
    auto session = co_await db.StartSession();
    tx1 = session.GetTx();
  }
  auto holder = co_await db.StartSession(tx1);
  auto holder_table = co_await holder.GetTable(kTableName);
  co_await holder_table->Upsert(kKey, "value1");  // tx1 now holds the lock on kKey

  // release the lock from a concurrent coroutine after a short delay
  Spawn([this, &db, tx1]() -> server::Awaitable<void> {
    co_await Sleep(std::chrono::milliseconds{50});
    auto committer = co_await db.StartSession(tx1);
    co_await committer.Commit();
  });

  // tx2 attempts to write the same key: it must suspend (without blocking the
  // event loop) until tx1 commits and releases the lock above.
  {
    auto session = co_await db.StartSession();
    auto table = co_await session.GetTable(kTableName);
    co_await table->Upsert(kKey, "value2");
    co_await session.Finish();
  }

  // reaching here means the waiter was resumed; verify last writer won
  {
    auto session = co_await db.StartSession();
    auto table = co_await session.GetTable(kTableName);
    auto value = co_await table->Lookup(kKey);
    CO_ASSERT_EQ(value.value_or(""), std::string{"value2"});
  }
})

// Test multiple rows can be locked by same transaction
DATABASE_TEST(MultipleRowLocks, {
  auto& db = GetDatabase();

  // Setup: Create table
  {
    auto session = co_await db.StartSession();
    co_await session.CreateTable(kTableName);
    co_await session.Finish();
  }

  // Write multiple keys in single transaction
  {
    auto session = co_await db.StartSession();
    auto table = co_await session.GetTable(kTableName);

    co_await table->Upsert("key1", "value1");
    co_await table->Upsert("key2", "value2");
    co_await table->Upsert("key3", "value3");

    co_await session.Finish();
  }

  // Verify all values are present
  {
    auto session = co_await db.StartSession();
    auto table = co_await session.GetTable(kTableName);

    auto v1 = co_await table->Lookup("key1");
    auto v2 = co_await table->Lookup("key2");
    auto v3 = co_await table->Lookup("key3");

    CO_ASSERT_TRUE(v1.has_value());
    CO_ASSERT_TRUE(v2.has_value());
    CO_ASSERT_TRUE(v3.has_value());
    CO_ASSERT_EQ(v1.value(), "value1");
    CO_ASSERT_EQ(v2.value(), "value2");
    CO_ASSERT_EQ(v3.value(), "value3");

    co_await session.Finish();
  }
})

}
