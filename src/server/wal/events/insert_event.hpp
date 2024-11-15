#pragma once

#include "event.hpp"

namespace structuredb::server::wal {

class InsertEvent : public Event {
public:
  explicit InsertEvent(const std::string& key, const std::string& value);

  static Awaitable<Event::Ptr> Parse(sdb::Reader& reader);

  EventType GetType() const override;

  Awaitable<void> Flush(sdb::Writer& writer) override;

  Awaitable<void> Apply(lsm::Lsm& lsm) override;
private:
  const std::string key_;
  const std::string value_;
};

}
