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

}
