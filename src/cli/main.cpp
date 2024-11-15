#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <string>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>

#include <table_service.pb.h>
#include <table_service.grpc.pb.h>


ABSL_FLAG(std::string, target, "localhost:50051", "Server address");


class TableServiceClient {
 public:
  explicit TableServiceClient(std::shared_ptr<grpc::Channel> channel)
      : stub_(structuredb::v1::Tables::NewStub(channel)) {}

  void Upsert(const std::string& key, const std::string& value) {
    structuredb::v1::UpsertTableRequest request;
    request.set_key(key);
    request.set_value(value);

    structuredb::v1::UpsertTableResponse response;
    grpc::ClientContext context;

    const auto status = stub_->Upsert(&context, request, &response);

    // Act upon its status.
    if (!status.ok()) {
      std::cerr << status.error_code() << ": " << status.error_message() << std::endl;
    }
  }

  std::optional<std::string> Lookup(const std::string& key) {
    structuredb::v1::LookupTableRequest request;
    request.set_key(key);

    structuredb::v1::LookupTableResponse response;
    grpc::ClientContext context;

    const auto status = stub_->Lookup(&context, request, &response);

    // Act upon its status.
    if (!status.ok()) {
      std::cerr << status.error_code() << ": " << status.error_message() << std::endl;
      return std::nullopt;
    }
    return response.value();
  }

 private:
  std::unique_ptr<structuredb::v1::Tables::Stub> stub_;
};

int main(int argc, char** argv) {
  const auto args = absl::ParseCommandLine(argc, argv);
  if (args.size() <= 2) {
    std::cerr << "Wrong usage\n";
    return 1;
  }

  const auto target_str = absl::GetFlag(FLAGS_target);
  TableServiceClient client(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  const auto cmd = std::string(args[1]);
  if (cmd == "UPSERT" && args.size() == 4) {
    client.Upsert(args[2], args[3]);
    return 0;
  }

  if (cmd == "LOOKUP" && args.size() == 3) {
    const auto result = client.Lookup(args[2]);
    std::cout << result.value_or("<null>") << std::endl;
    return 0;
  }

  std::cerr << "Wrong usage\n";
  return 1;
}
