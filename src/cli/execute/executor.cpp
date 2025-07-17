#include "executor.hpp"

#include <utils/split.hpp>

namespace structuredb::cli {

Executor::Executor(client::database::AutoCommitDatabase::Ptr db)
    : db_{std::move(db)}
{}

void Executor::Execute(const std::string_view& command) {
  if (command == "BEGIN") {
    if (tx_) {
      throw std::runtime_error("Transaction already in progress. Use COMMIT or ROLLBACK first.");
    }
    tx_ = db_->Begin();
  }
  if (command == "COMMIT") {
    EnsureInTransaction();
    tx_->Commit();
    tx_.reset();
  }
  if (command == "ROLLBACK") {
    EnsureInTransaction();
    tx_->Rollback();
    tx_.reset();
  }

  const auto tokens = cli::Split(command, ' ');
  if (tokens.size() == 3 && tokens[0] == "CREATE" && tokens[1] == "TABLE") {
    const auto& table_name = tokens[2];
    db_->CreateTable(std::string{table_name});
  }

  if (tokens.size() < 2) {
    throw std::runtime_error("Invalid command.");
  }

  const auto& cmd = tokens[0];
  const std::string table_name{tokens[1]};
  const auto table = tx_ ? tx_->Table(table_name) : db_->Table(table_name);

  if (cmd == "UPSERT") {
    if (tokens.size() != 4) {
      throw std::runtime_error("UPSERT command requires 3 arguments: <table_name> <key> <value>");
    }
    table->Upsert(std::string{tokens[2]}, std::string{tokens[3]});
  }

  if (cmd == "LOOKUP") {
    if (tokens.size() != 3) {
      throw std::runtime_error("LOOKUP command requires 2 arguments: <table_name> <key>");
    }
    auto value = table->Lookup(std::string{tokens[2]});
    if (value) {
      std::cout << "Value: " << *value << std::endl;
    } else {
      std::cout << "Key not found." << std::endl;
    }
  }

  if (cmd == "DELETE") {
    if (tokens.size() != 3) {
      throw std::runtime_error("DELETE command requires 2 arguments: <table_name> <key>");
    }
    table->Delete(std::string{tokens[2]});
    std::cout << "Key deleted." << std::endl;
  }
}

void Executor::EnsureInTransaction() const {
  if (!tx_) {
    throw std::runtime_error("No transaction in progress. Use BEGIN to start a transaction.");
  }
}

}
