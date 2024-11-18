#pragma once

#include <sdb/writer.hpp>
#include <sdb/reader.hpp>

namespace structuredb::server::database {
class Database;
}

namespace structuredb::server::wal {

enum class EventType : int64_t {
  kInvalid = 0,
  kInsert = 1,
};

class Event {
public:
  using Ptr = std::unique_ptr<Event>;

  virtual EventType GetType() const = 0;

  virtual Awaitable<void> Flush(sdb::Writer& writer) = 0;

  virtual Awaitable<void> Apply(database::Database&) = 0;

  virtual ~Event() = default;
};

}
