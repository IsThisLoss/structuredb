#include <iostream>
#include <memory>
#include <string>
#include <chrono>

#include <grpcpp/grpcpp.h>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>

#include <table_service.pb.h>
#include <table_service.grpc.pb.h>
#include <transaction_service.pb.h>
#include <transaction_service.grpc.pb.h>


ABSL_FLAG(std::string, target, "localhost:50051", "Server address");
ABSL_FLAG(std::optional<std::string>, tx, std::nullopt, "Transaction number");


class TableServiceClient {
 public:
  explicit TableServiceClient(std::shared_ptr<grpc::Channel> channel)
      : stub_(structuredb::v1::Tables::NewStub(channel)) {}

  std::string Upsert(const std::optional<std::string>& tx, const std::string& table, const std::string& key, const std::string& value) {
    structuredb::v1::UpsertTableRequest request;
    if (tx.has_value()) {
      request.set_tx(tx.value());
    }
    request.set_table(table);
    request.set_key(key);
    request.set_value(value);

    structuredb::v1::UpsertTableResponse response;
    grpc::ClientContext context;

    const auto status = stub_->Upsert(&context, request, &response);

    // Act upon its status.
    if (!status.ok()) {
      std::cerr << status.error_code() << ": " << status.error_message() << std::endl;
    }

    return response.tx();
  }

  std::optional<std::string> Lookup(const std::optional<std::string>& tx, const std::string& table, const std::string& key) {
    structuredb::v1::LookupTableRequest request;
    if (tx.has_value()) {
      request.set_tx(tx.value());
    }
    request.set_table(table);
    request.set_key(key);

    structuredb::v1::LookupTableResponse response;
    grpc::ClientContext context;

    const auto status = stub_->Lookup(&context, request, &response);

    // Act upon its status.
    if (!status.ok()) {
      std::cerr << status.error_code() << ": " << status.error_message() << std::endl;
      return std::nullopt;
    }
    if (!response.has_value()) {
      return std::nullopt;
    }
    return response.value();
  }

  void Delete(const std::optional<std::string>& tx, const std::string& table, const std::string& key) {
    structuredb::v1::DeleteTableRequest request;
    if (tx.has_value()) {
      request.set_tx(tx.value());
    }
    request.set_table(table);
    request.set_key(key);

    structuredb::v1::DeleteTableResponse response;
    grpc::ClientContext context;

    const auto status = stub_->Delete(&context, request, &response);

    // Act upon its status.
    if (!status.ok()) {
      std::cerr << status.error_code() << ": " << status.error_message() << std::endl;
    }
  }

  std::string CreateTable(const std::optional<std::string>& tx, const std::string& name) {
    structuredb::v1::CreateTableRequest request;
    if (tx.has_value()) {
      request.set_tx(tx.value());
    }
    request.set_name(name);

    structuredb::v1::CreateTableResponse response;
    grpc::ClientContext context;

    const auto status = stub_->CreateTable(&context, request, &response);

    // Act upon its status.
    if (!status.ok()) {
      std::cerr << status.error_code() << ": " << status.error_message() << std::endl;
    }

    return response.tx();
  }

  std::string DropTable(const std::optional<std::string>& tx, const std::string& name) {
    structuredb::v1::DropTableRequest request;
    if (tx.has_value()) {
      request.set_tx(tx.value());
    }
    request.set_name(name);

    structuredb::v1::DropTableResponse response;
    grpc::ClientContext context;

    const auto status = stub_->DropTable(&context, request, &response);

    // Act upon its status.
    if (!status.ok()) {
      std::cerr << status.error_code() << ": " << status.error_message() << std::endl;
    }

    return response.tx();
  }

 private:
  std::unique_ptr<structuredb::v1::Tables::Stub> stub_;
};

class TransactionServiceClient {
 public:
  explicit TransactionServiceClient(std::shared_ptr<grpc::Channel> channel)
      : stub_(structuredb::v1::Transactions::NewStub(channel)) {}

  std::string Begin() {
    structuredb::v1::BeginRequest request;

    structuredb::v1::BeginResponse response;
    grpc::ClientContext context;

    const auto status = stub_->Begin(&context, request, &response);

    // Act upon its status.
    if (!status.ok()) {
      std::cerr << status.error_code() << ": " << status.error_message() << std::endl;
    }

    return response.tx();
  }

  void Commit(const std::string& tx) {
    structuredb::v1::CommitRequest request;
    request.set_tx(tx);

    structuredb::v1::CommitResponse response;
    grpc::ClientContext context;

    const auto status = stub_->Commit(&context, request, &response);

    // Act upon its status.
    if (!status.ok()) {
      std::cerr << status.error_code() << ": " << status.error_message() << std::endl;
    }
  }

 private:
  std::unique_ptr<structuredb::v1::Transactions::Stub> stub_;
};

int main(int argc, char** argv) {
  const auto args = absl::ParseCommandLine(argc, argv);
  if (args.size() <= 1) {
    std::cerr << "Wrong usage\n";
    return 1;
  }

  const auto target_str = absl::GetFlag(FLAGS_target);
  const auto tx = absl::GetFlag(FLAGS_tx);
  TableServiceClient client(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
  TransactionServiceClient tx_client(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  const auto cmd = std::string(args[1]);
  if (cmd == "UPSERT" && args.size() == 5) {
    std::cout << "Tx: " << client.Upsert(tx, args[2], args[3], args[4]) << std::endl;
    return 0;
  }

  if (cmd == "LOOKUP" && args.size() == 4) {
    auto start = std::chrono::steady_clock::now();
    const auto result = client.Lookup(tx, args[2], args[3]);
    std::cout << result.value_or("<null>") << std::endl;
    auto end = std::chrono::steady_clock::now();
    std::cout << "Elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << std::endl;
    return 0;
  }
  if (cmd == "DELETE" && args.size() == 4) {
    client.Delete(tx, args[2], args[3]);
    return 0;
  }

  if (cmd == "BEGIN" && args.size() == 2) {
    const auto result = tx_client.Begin();
    std::cout << result << std::endl;
    return 0;
  }

  if (cmd == "COMMIT" && args.size() == 3) {
    tx_client.Commit(args[2]);
    std::cout << "Commited" << std::endl;
    return 0;
  }

  if (cmd == "CREATE" && args.size() == 3) {
    std::cout << "Tx: " << client.CreateTable(tx, args[2]) << std::endl;
    return 0;
  }

  if (cmd == "DROP" && args.size() == 3) {
    std::cout << "Tx: " << client.DropTable(tx, args[2]) << std::endl;
    return 0;
  }

  std::cerr << "Wrong usage\n";
  return 1;
}
