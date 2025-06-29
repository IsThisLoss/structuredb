#include "database.hpp"

#include <functional>

#include <grpcpp/channel.h>

#include <connection/connection.hpp>

#include "transaction.hpp"

namespace structuredb::client::database {

class AutoCommitDatabase : public Database {
public:
  using Ptr = std::shared_ptr<AutoCommitDatabase>;

  using TxCallback = std::function<void(Transaction::Ptr)>;

  explicit AutoCommitDatabase() = default;

  AutoCommitDatabase(const AutoCommitDatabase&) = delete;

  virtual Transaction::Ptr Begin() const = 0;

  virtual void WithTransaction(const TxCallback& cb) const = 0;
};

AutoCommitDatabase::Ptr MakeAutoCommitDatabase(connection::Connection::Ptr connection);

}
