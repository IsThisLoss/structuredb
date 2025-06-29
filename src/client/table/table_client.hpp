#pragma once

#include <memory>
#include <string>
#include <vector>

#include <connection/connection.hpp>

namespace structuredb::client::table {

// @brief interface of single table client
class TableClient {
public:
  using Ptr = std::shared_ptr<TableClient>;

  // @brief represents row of a table
  struct Record {
    std::string key;
    std::string value;
  };

  explicit TableClient() = default;

  TableClient(const TableClient&) = delete;

  // @brief sets value to the key 
  virtual void Upsert(const std::string& key, const std::string& value) const = 0;

  // @brief returns value by key
  virtual std::optional<std::string> Lookup(const std::string& key) const = 0;

  // @brief returns all record withn given range
  //
  // if one of params was not provided, returns open range
  // for example, Scan(std::nullopt, std::nullopt) returns the whole table
  virtual std::vector<Record> Scan(const std::optional<std::string>& lower_bound, std::optional<std::string>& upper_bound) const = 0;

  virtual void Delete(const std::string& key) const = 0;

  virtual ~TableClient() = default;
};

TableClient::Ptr MakeTableClient(connection::Connection::Ptr connection, std::string table_name, std::optional<std::string> tx);

}
