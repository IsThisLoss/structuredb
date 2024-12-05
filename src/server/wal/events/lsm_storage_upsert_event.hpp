#pragma once

#include "event.hpp"

#include <lsm/types.hpp>

namespace structuredb::server::wal {

class LsmStorageUpsertEvent : public Event {
public:
  explicit LsmStorageUpsertEvent(
      const std::string& storage_id,
      const lsm::Sequence seq_no,
      const std::string& key,
      const std::string& value
  );

  static Awaitable<Event::Ptr> Parse(sdb::Reader& reader);

  EventType GetType() const override;

  Awaitable<void> Flush(sdb::Writer& writer) override;

  Awaitable<void> Apply(database::Database&) override;
private:
  const std::string storage_id_;
  const lsm::Sequence seq_no_;
  const std::string key_;
  const std::string value_;
};

}
