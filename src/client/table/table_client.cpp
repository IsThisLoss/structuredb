#include "table_client.hpp"

#include <utils/status_check.hpp>

namespace structuredb::client::table {

namespace {

class TableClientImpl : public TableClient {
public:
  explicit TableClientImpl(connection::Connection::Ptr connection, std::string table_name, std::optional<std::string> tx)
      : connection_(std::move(connection)), table_name_(std::move(table_name)), tx_(std::move(tx)) {}

  void Upsert(const std::string& key, const std::string& value) const override {
    v1::UpsertTableRequest req{};
    req.set_table(table_name_);
    if (tx_.has_value()) {
      req.set_tx(tx_.value());
    }
    req.set_key(key);
    req.set_value(value);

    v1::UpsertTableResponse res{};
    const auto status = connection_->tables->Upsert(&connection_->context, req, &res);
    CheckStatus(status);
  }

  std::optional<std::string> Lookup(const std::string& key) const override {
    v1::LookupTableRequest req{};
    req.set_table(table_name_);
    if (tx_.has_value()) {
      req.set_tx(tx_.value());
    }
    req.set_key(key);

    v1::LookupTableResponse res{};
    const auto status = connection_->tables->Lookup(&connection_->context, req, &res);
    CheckStatus(status);

    if (res.has_value()) {
      return res.value();
    }
    return std::nullopt;
  }

  std::vector<Record> Scan(const std::optional<std::string>& lower_bound, std::optional<std::string>& upper_bound) const override {
    v1::ScanTableRequest req{};
    req.set_table(table_name_);
    if (tx_.has_value()) {
      req.set_tx(tx_.value());
    }
    if (lower_bound.has_value()) {
      req.set_lower_bound(lower_bound.value());
    }
    if (upper_bound.has_value()) {
      req.set_upper_bound(upper_bound.value());
    }

    v1::ScanTableResponse res{};
    const auto status = connection_->tables->Scan(&connection_->context, req, &res);
    CheckStatus(status);

    std::vector<Record> records;
    records.reserve(res.records_size());
    for (const auto& record : res.records()) {
      records.push_back(Record{
          .key = record.key(),
          .value = record.value(),
      });
    }
    return records;
  }

  void Delete(const std::string& key) const override {
    v1::DeleteTableRequest req{};
    req.set_table(table_name_);
    if (tx_.has_value()) {
      req.set_tx(tx_.value());
    }
    req.set_key(key);

    v1::DeleteTableResponse res{};
    const auto status = connection_->tables->Delete(&connection_->context, req, &res);
    CheckStatus(status);
  }

private:
  connection::Connection::Ptr connection_;
  std::string table_name_;
  std::optional<std::string> tx_;
};

}

TableClient::Ptr MakeTableClient(connection::Connection::Ptr connection, std::string table_name, std::optional<std::string> tx) {
  return std::make_shared<TableClientImpl>(std::move(connection), std::move(table_name), std::move(tx));
}

}
