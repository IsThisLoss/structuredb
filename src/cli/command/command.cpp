#include "command.hpp"

#include "manager.hpp"

#include <utils/split.hpp>

namespace structuredb::cli {

namespace {

client::database::Database::Ptr GetDatabase(Context& context) {
  if (context.tx) {
    return context.tx;
  }
  return context.db;
}

class CreateTableComman : public Command {
public:
  constexpr static const char* kName = "CREATE TABLE";
  constexpr static const char* kHints = " <table_name>";

  static Command::Ptr TryParse(const std::vector<std::string>& tokens) {
    if (!tokens.empty() && tokens[0] == "CREATE" && tokens[1] == "TABLE") {
      if (tokens.size() != 3) {
        throw std::runtime_error("CREATE TABLE command requires exactly one table name.");
      }
      return std::make_unique<CreateTableComman>(tokens[2]);
    }
    return nullptr;
  }

  explicit CreateTableComman(std::string table_name)
      : table_name_{std::move(table_name)} {}

  void Execute(Context& context) const override {
    const auto db = GetDatabase(context);
    db->CreateTable(table_name_);
  }

private:
  std::string table_name_;
};

class DropTableCommand : public Command {
public:
  constexpr static const char* kName = "DROP TABLE";
  constexpr static const char* kHints = " <table_name>";

  static Command::Ptr TryParse(const std::vector<std::string>& tokens) {
    if (!tokens.empty() && tokens[0] == "DROP" && tokens[1] == "TABLE") {
      if (tokens.size() != 3) {
        throw std::runtime_error("DROP TABLE command requires exactly one table name.");
      }
      return std::make_unique<DropTableCommand>(tokens[2]);
    }
    return nullptr;
  }

  explicit DropTableCommand(std::string table_name)
      : table_name_{std::move(table_name)} {}

  void Execute(Context& context) const override {
    const auto db = GetDatabase(context);
    db->DropTable(table_name_);
  }
private:
  std::string table_name_;
};

class BeginCommand : public Command {
public:
  constexpr static const char* kName = "BEGIN";
  constexpr static const char* kHints = "";

  static Command::Ptr TryParse(const std::vector<std::string>& tokens) {
    if (!tokens.empty() && tokens[0] == "BEGIN") {
      if (tokens.size() != 1) {
        throw std::runtime_error("BEGIN command does not require any arguments.");
      }
      return std::make_unique<BeginCommand>();
    }
    return nullptr;
  }

  void Execute(Context& context) const override {
    if (context.tx) {
      throw std::runtime_error("Transaction already in progress. Use COMMIT or ROLLBACK first.");
    }
    context.tx = context.db->Begin();
    std::cout << context.tx->GetId() << std::endl;
  }
};

class CommitCommand : public Command {
public:
  constexpr static const char* kName = "COMMIT";
  constexpr static const char* kHints = "";

  static Command::Ptr TryParse(const std::vector<std::string>& tokens) {
    if (!tokens.empty() && tokens[0] == "COMMIT") {
      if (tokens.size() != 1) {
        throw std::runtime_error("COMMIT command does not require any arguments.");
      }
      return std::make_unique<CommitCommand>();
    }
    return nullptr;
  }

  void Execute(Context& context) const override {
    if (!context.tx) {
      throw std::runtime_error("No transaction in progress. Use BEGIN first.");
    }
    context.tx->Commit();
    std::cout << context.tx->GetId() << std::endl;
    context.tx.reset();
  }
};

class UpsertCommand : public Command {
public:
  constexpr static const char* kName = "UPSERT";
  constexpr static const char* kHints = " <table_name> <key> <value>";

  static Command::Ptr TryParse(const std::vector<std::string>& tokens) {
    if (!tokens.empty() && tokens[0] == "UPSERT") {
      if (tokens.size() != 4) {
        throw std::runtime_error("UPSERT command requires exactly three arguments: <table_name> <key> <value>");
      }
      return std::make_unique<UpsertCommand>(tokens[1], tokens[2], tokens[3]);
    }
    return nullptr;
  }

  explicit UpsertCommand(std::string table_name, std::string key, std::string value)
      : table_name_{std::move(table_name)},
    key_{std::move(key)},
    value_{std::move(value)} {}

  void Execute(Context& context) const override {
    const auto db = GetDatabase(context);
    auto table = db->Table(table_name_);
    table->Upsert(key_, value_);
  }

private:
  std::string table_name_;
  std::string key_;
  std::string value_;
};

class LookupCommand : public Command {
public:
  constexpr static const char* kName = "LOOKUP";
  constexpr static const char* kHints = " <table_name> <key>";

  static Command::Ptr TryParse(const std::vector<std::string>& tokens) {
    if (!tokens.empty() && tokens[0] == "LOOKUP") {
      if (tokens.size() != 3) {
        throw std::runtime_error("LOOKUP command requires exactly two arguments: <table_name> <key>");
      }
      return std::make_unique<LookupCommand>(tokens[1], tokens[2]);
    }
    return nullptr;
  }

  explicit LookupCommand(std::string table_name, std::string key)
      : table_name_{std::move(table_name)}, key_{std::move(key)} {}

  void Execute(Context& context) const override {
    const auto db = GetDatabase(context);
    const auto table = db->Table(table_name_);
    const auto value = table->Lookup(key_);
    if (value.has_value()) {
      std::cout << value.value() << std::endl;
    } else {
      std::cerr << "Key "<< key_ << " was not found." << std::endl;
    }
  }
private:
  std::string table_name_;
  std::string key_;
};

class DeleteCommand : public Command {
public:
  constexpr static const char* kName = "DELETE";
  constexpr static const char* kHints = " <table_name> <key>";

  static Command::Ptr TryParse(const std::vector<std::string>& tokens) {
    if (tokens.size() >= 3 && tokens[0] == "DELETE") {
      if (tokens.size() != 3) {
        throw std::runtime_error("DELETE command requires exactly two arguments: <table_name> <key>");
      }
      return std::make_unique<DeleteCommand>(tokens[1], tokens[2]);
    }
    return nullptr;
  }

  explicit DeleteCommand(std::string table_name, std::string key)
      : table_name_{std::move(table_name)}, key_{std::move(key)} {}

  void Execute(Context& context) const override {
    const auto db = GetDatabase(context);
    auto table = db->Table(table_name_);
    table->Delete(key_);
    std::cerr << "Key " << key_ << " was deleted." << std::endl;
  }
private:
  std::string table_name_;
  std::string key_;
};

} // namespace

void RegisterCommands(CommandManager& manager) {
  manager
    .Add(CreateTableComman::kName, CreateTableComman::TryParse, CreateTableComman::kHints)
    .Add(DropTableCommand::kName, DropTableCommand::TryParse, DropTableCommand::kHints)
    .Add(BeginCommand::kName, BeginCommand::TryParse, BeginCommand::kHints)
    .Add(CommitCommand::kName, CommitCommand::TryParse, CommitCommand::kHints)
    .Add(UpsertCommand::kName, UpsertCommand::TryParse, UpsertCommand::kHints)
    .Add(LookupCommand::kName, LookupCommand::TryParse, LookupCommand::kHints)
    .Add(DeleteCommand::kName, DeleteCommand::TryParse, DeleteCommand::kHints);
}

}
