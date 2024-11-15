#pragma once

#include <sys/types.h>
#include <lsm/lsm.hpp>
#include <sdb/writer.hpp>
#include <sdb/reader.hpp>

namespace structuredb::server::wal {

enum class EventType : int64_t {
  kInvalid = 1,
  kInsert = 1,
};

class Event {
public:
  using Ptr = std::unique_ptr<Event>;

  virtual EventType GetType() const = 0;

  virtual Awaitable<void> Flush(sdb::Writer& writer) = 0;

  virtual Awaitable<void> Apply(lsm::Lsm& lsm) = 0;

  virtual ~Event() = default;
};

}
