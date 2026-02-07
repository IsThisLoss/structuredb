#include "tx_storage_commit_event.hpp"

#include "database/database.hpp"

namespace structuredb::server::wal {

TxStorageCommitEvent::TxStorageCommitEvent(transaction::TransactionId tx_id)
  : tx_id_(tx_id)
{}

Event::Ptr TxStorageCommitEvent::Parse(sdb::Reader& reader) {
  const auto tx_id = reader.ReadInt();
  return std::make_unique<TxStorageCommitEvent>(tx_id);
}

EventType TxStorageCommitEvent::GetType() const {
  return EventType::kTxStorageCommit;
}

Awaitable<void> TxStorageCommitEvent::Apply(database::Database& db) {
  co_await db.GetTransactionStorage()->RecoverFromLog(tx_id_);
}

Awaitable<bool> TxStorageCommitEvent::IsPersistent(database::Database&) {
  // TxStorageCommitEvent is persistent, because it should be applied during recovery
  co_return true;
}

void TxStorageCommitEvent::Flush(sdb::Writer& writer) {
  writer.WriteInt(tx_id_);
}

}