#pragma once

#include "event.hpp"

namespace structuredb::server::wal {

Awaitable<Event::Ptr> ParseEvent(sdb::Reader& reader);

Awaitable<void> FlushEvent(sdb::Writer& writer, const Event::Ptr& event);

}
