#include "common.hpp"

#include <database/database.hpp>

namespace structuredb::tests {

namespace {

static const std::string kTableName = "test_table";
static const std::string kKey = "test_key";
static const std::string kValue = "test_value";

}

TEST_F(DatabaseTest, CreateDropTable) {
  auto& db = GetDatabase();

  // create table
  {
    auto session = db.StartSession();
    session.CreateTable(kTableName);
    session.Finish();
  }

  // get table
  {
    auto session = db.StartSession();
    auto table = session.GetTable(kTableName);
    session.Finish();
    ASSERT_NE(table, nullptr);
  }

  // drop table
  {
    auto session = db.StartSession();
    session.DropTable(kTableName);
    session.Finish();
  }

  // get table
  {
    auto session = db.StartSession();
    auto table = session.GetTable(kTableName);
    session.Finish();
    ASSERT_EQ(table, nullptr);
  }
}

TEST_F(DatabaseTest, UpsertLookupDelete) {
  auto& db = GetDatabase();

  // create table
  {
    auto session = db.StartSession();
    session.CreateTable(kTableName);
    session.Finish();
  }

  // upsert
  {
    auto session = db.StartSession();
    auto table = session.GetTable(kTableName);
    table->Upsert(kKey, kValue);
    session.Finish();
  }
 
  // lookup
  {
    auto session = db.StartSession();
    auto table = session.GetTable(kTableName);
    auto value = table->Lookup(kKey);
    session.Finish();
   
    ASSERT_TRUE(value.has_value());
    ASSERT_EQ(value.value(), kValue);
  }

  // delete
  {
    auto session = db.StartSession();
    auto table = session.GetTable(kTableName);
    table->Delete(kKey);
    session.Finish();
  }

  // lookup
  {
    auto session = db.StartSession();
    auto table = session.GetTable(kTableName);
    auto value = table->Lookup(kKey);
    session.Finish();
    ASSERT_FALSE(value.has_value());
  }
}

TEST_F(DatabaseTest, TxIsolation) {
  auto& db = GetDatabase();

  // create table
  auto session = db.StartSession();
  session.CreateTable(kTableName);
  session.Finish();
 
  // begin transaction
  server::transaction::TransactionId tx{};
  {
    auto session = db.StartSession();
    tx = session.GetTx();
  }

  // upsert and lookup in transaction
  {
    auto session = db.StartSession(tx);
    auto table = session.GetTable(kTableName);
    table->Upsert(kKey, kValue);
    auto value = table->Lookup(kKey);

    ASSERT_TRUE(value.has_value());
    ASSERT_EQ(value.value(), kValue);
  }
 
  // lookup outside of transaction
  {
    auto session = db.StartSession();
    auto table = session.GetTable(kTableName);
    auto value = table->Lookup(kKey);
    session.Finish();
    ASSERT_FALSE(value.has_value());
  }

  // commit transaction
  // lookup again
  {
    auto session = db.StartSession(tx);
    session.Commit();
    session.Finish();
  }

  {
    auto session = db.StartSession();
    auto table = session.GetTable(kTableName);
    auto value = table->Lookup(kKey);
    session.Finish();
    ASSERT_TRUE(value.has_value());
    ASSERT_EQ(value.value(), kValue);
  }
}

TEST_F(DatabaseTest, RangeScan) {
  const int64_t kSize = 100;
  const int64_t kLowerBound = 20;
  const int64_t kUpperBound = 40;
  auto& db = GetDatabase();

  // create table and insert 100 key-values into it
  {
    auto session = db.StartSession();
    session.CreateTable(kTableName);
    auto table = session.GetTable(kTableName);
    for (int64_t i = 0; i < kSize; i++) {
      const auto key = fmt::format("{:02}", i);
      const auto value = fmt::format("{:02}", -1 * i);
      table->Upsert(key, value);
    }
    session.Finish();
  }

  auto session = db.StartSession();
  auto table = session.GetTable(kTableName);
  table->Upsert(kKey, kValue);
  auto iter = table->Scan(std::to_string(kLowerBound), std::to_string(kUpperBound));
  session.Finish();

  std::vector<std::pair<std::string, std::string>> result;
  while (iter->HasMore()) {
    auto row = iter->Next();
    result.emplace_back(std::move(row.key), std::move(row.value));
  }

  const int64_t kExpectedSize = kUpperBound - kLowerBound +1; // [20; 40], include right border

  ASSERT_EQ(result.size(), kExpectedSize);

  int64_t idx = 0;
  for (int64_t i = kLowerBound; i <= kUpperBound; i++) {
    const auto expected_key = fmt::format("{:02}", i);
    ASSERT_EQ(result[idx].first, expected_key);
    const auto expected_value = fmt::format("{:02}", -1 * i);
    ASSERT_EQ(result[idx].second, expected_value);
    idx++;
  }
}

TEST_F(DatabaseTest, Compaction) {
  const int64_t kSize = 1000;
  auto& db = GetDatabase();

  // create table and insert 100 key-values into it
  {
    auto session = db.StartSession();
    session.CreateTable(kTableName);
    auto table = session.GetTable(kTableName);
    for (int64_t i = 0; i < kSize; i++) {
      const auto key = fmt::format("{:03}", i);
      const auto value = fmt::format("{:03}", -1 * i);
      table->Upsert(key, value);
    }
    session.Finish();
  }

  {
    auto session = db.StartSession();
    auto table = session.GetTable(kTableName);

    const int old_ss_tables_count = session.CountSSTables(kTableName);
    ASSERT_TRUE(old_ss_tables_count > 1);

    table->Compact();
    const int new_ss_tables_count = session.CountSSTables(kTableName);
    ASSERT_EQ(new_ss_tables_count, 1);
    ASSERT_TRUE(new_ss_tables_count < old_ss_tables_count) << "new: " << new_ss_tables_count << " old: " << old_ss_tables_count;
    session.Finish();
  }

  auto session = db.StartSession();
  auto table = session.GetTable(kTableName);
  auto iter = table->Scan(std::nullopt, std::nullopt);

  // assert all keys stil in table
  // TODO THIS IS BROKEN
  int count = 0;
  while (iter->HasMore()) {
    auto row = iter->Next();
    const auto expected_key = fmt::format("{:03}", count);
    ASSERT_EQ(row.key, expected_key);
    const auto expected_value = fmt::format("{:03}", -1 * count);
    ASSERT_EQ(row.value, expected_value);
    count++;
  }
  ASSERT_EQ(count, kSize);
  session.Finish();
}

}
