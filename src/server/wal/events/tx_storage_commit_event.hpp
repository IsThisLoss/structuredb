#pragma once

#include "event.hpp"
#include <io/types.hpp>
#include <lsm/types.hpp>
#include <transaction/types.hpp>

namespace structuredb::server::wal {

class TxStorageCommitEvent : public Event {
public:
  explicit TxStorageCommitEvent(transaction::TransactionId tx_id);
  
  static Event::Ptr Parse(sdb::Reader& reader);
  
  EventType GetType() const override;

  Awaitable<void> Apply(database::Database &) override;

  Awaitable<bool> IsPersistent(database::Database&) override;

  void Flush(sdb::Writer& writer) override;
private:
  const transaction::TransactionId tx_id_;
};

}