#pragma once

#include "event.hpp"

#include <lsm/types.hpp>

namespace structuredb::server::wal {

class LoggedTableUpsertEvent : public Event {
public:
  explicit LoggedTableUpsertEvent(
      const std::string& table_name,
      const lsm::Sequence seq_no,
      const std::string& key,
      const std::string& value
  );

  static Awaitable<Event::Ptr> Parse(sdb::Reader& reader);

  EventType GetType() const override;

  Awaitable<void> Flush(sdb::Writer& writer) override;

  Awaitable<void> Apply(database::Database&) override;
private:
  const std::string table_name_;
  const lsm::Sequence seq_no_;
  const std::string key_;
  const std::string value_;
};

}