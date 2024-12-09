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
  Run(
    [&db]() -> server::Awaitable<void> {
      auto session = co_await db.StartSession();
      co_await session.CreateTable(kTableName);
      co_await session.Finish();
    }()
  );

  // get table
  auto table = Run(
    [&db]() -> server::Awaitable<server::table::Table::Ptr> {
      auto session = co_await db.StartSession();
      auto result = co_await session.GetTable(kTableName);
      co_await session.Finish();
      co_return result;
    }()
  );

  ASSERT_NE(table, nullptr);

  // drop table
  Run(
    [&db]() -> server::Awaitable<void> {
      auto session = co_await db.StartSession();
      co_await session.DropTable(kTableName);
      co_await session.Finish();
    }()
  );

  // get table
  table = Run(
    [&db]() -> server::Awaitable<server::table::Table::Ptr> {
      auto session = co_await db.StartSession();
      auto result = co_await session.GetTable(kTableName);
      co_await session.Finish();
      co_return result;
    }()
  );

  ASSERT_EQ(table, nullptr);
}

TEST_F(DatabaseTest, UpsertLookupDelete) {
  auto& db = GetDatabase();

  // create table
  Run(
    [&db]() -> server::Awaitable<void> {
      auto session = co_await db.StartSession();
      co_await session.CreateTable(kTableName);
      co_await session.Finish();
    }()
  );

  // upsert
  Run(
    [&db]() -> server::Awaitable<void> {
      auto session = co_await db.StartSession();
      auto table = co_await session.GetTable(kTableName);
      co_await table->Upsert(kKey, kValue);
      co_await session.Finish();
    }()
  );
 
  // lookup
  auto value = Run(
    [&db]() -> server::Awaitable<std::optional<std::string>> {
      auto session = co_await db.StartSession();
      auto table = co_await session.GetTable(kTableName);
      auto result = co_await table->Lookup(kKey);
      co_await session.Finish();
      co_return result;
    }()
  );
 
  ASSERT_TRUE(value.has_value());
  ASSERT_EQ(value.value(), kValue);

  // delete
  Run(
    [&db]() -> server::Awaitable<void> {
      auto session = co_await db.StartSession();
      auto table = co_await session.GetTable(kTableName);
      co_await table->Delete(kKey);
      co_await session.Finish();
    }()
  );

  // lookup
  value = Run(
    [&db]() -> server::Awaitable<std::optional<std::string>> {
      auto session = co_await db.StartSession();
      auto table = co_await session.GetTable(kTableName);
      auto result = co_await table->Lookup(kKey);
      co_await session.Finish();
      co_return result;
    }()
  );
 
  ASSERT_FALSE(value.has_value());
}

}
