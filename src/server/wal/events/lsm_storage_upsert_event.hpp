#pragma once

#include "event.hpp"

#include <string>
#include <io/types.hpp>
#include <lsm/types.hpp>

namespace structuredb::server::wal {

class LsmStorageUpsertEvent : public Event {
public:
  explicit LsmStorageUpsertEvent(
      std::string storage_id,
      lsm::Sequence seq_no,
      std::string key,
      std::string value
  );

  static Event::Ptr Parse(sdb::Reader& reader);

  EventType GetType() const override;

  void Flush(sdb::Writer& writer) override;

  Awaitable<void> Apply(database::Database&) override;

  Awaitable<bool> IsPersistent(database::Database&) override;
private:
  const std::string storage_id_;
  const lsm::Sequence seq_no_;
  const std::string key_;
  const std::string value_;
};

}
