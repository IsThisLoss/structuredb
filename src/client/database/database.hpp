#pragma once

#include <memory>
#include <string>

#include <table/table_client.hpp>

namespace structuredb::client::database {

// @brief interface of database client
class Database {
public:
  using Ptr = std::shared_ptr<Database>;

  explicit Database() = default;

  Database(const Database&) = delete;

  virtual void CreateTable(const std::string& table_name) const = 0;

  virtual void DropTable(const std::string& table_name) const = 0;

  virtual table::TableClient::Ptr Table(const std::string& table_name) const = 0;

  virtual ~Database() = default;
};

}
