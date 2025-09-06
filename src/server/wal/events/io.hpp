#pragma once

#include "event.hpp"

namespace structuredb::server::wal {

Event::Ptr ParseEvent(sdb::Reader& reader);

void FlushEvent(sdb::Writer& writer, const Event::Ptr& event);

}
